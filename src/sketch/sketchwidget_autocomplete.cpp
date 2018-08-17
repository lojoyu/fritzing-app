#include <QtCore>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "sketchwidget.h"
#include "../autocomplete/autocompleter.h"
#include "../debugdialog.h"
#include "../connectors/connectoritem.h"
#include "../waitpushundostack.h"
#include "../items/wire.h"

typedef QPair<long, long> LongLongPair;
typedef QPair<QString, QString> StringPair;

void SketchWidget::setAutoComplete(bool autoComplete) {
	m_autoComplete = autoComplete;
}

void SketchWidget::addSetToSet(QSharedPointer<ModelSet> modelSet, QSharedPointer<SetConnection> setconnection, bool transparent) {
    addModelSet(modelSet, transparent);
    addSetConnection(setconnection, transparent);
    modelSet->addSetConnection(setconnection);
}

void SketchWidget::addSetConnection(QSharedPointer<SetConnection> setconnection, bool transparent) {
    QSharedPointer<ModelSet> from = setconnection->getFromModelSet();
    QSharedPointer<ModelSet> to = setconnection->getToModelSet();
    if (from.isNull() || to.isNull()) return;
    if (from->single()) {
        ItemBase * keyItem = from->keyItem();
        if (keyItem == NULL) {
            long id = from->keyId();
            keyItem = findItem(id);
        }
        if (keyItem != NULL) {
            from->setKeyItem(keyItem);
            keyItem->setModelSet(from);
        }
    }

	QList<QPair<QString, QString>> connectionList = setconnection->getConnectionList();
    foreach(StringPair c, connectionList) {
        QPair<ItemBase *, QString> p1 = from->getItemAndCID(c.first);
        QPair<ItemBase *, QString> p2 = to->getItemAndCID(c.second);
        if (p1.first == NULL|| p2.first == NULL || p1.second == "" ||  p2.second == "") continue;
        ItemBase * wire = addSetWire(p1.first, p1.second, p2.first, p2.second, transparent);
        if (wire == NULL) continue;
		setconnection->appendWireList(wire);
	}

}

void SketchWidget::addModelSet(QSharedPointer<ModelSet> modelSet, bool transparent) {
    //m_savedModelSet.append(modelSet);
    if (modelSet == m_prevModelSet) {
        if (!transparent) {
            setOpacity(modelSet);
            confirmSelect(modelSet);
        } else {
            removePrevSetConnection();
        }
        return;
    }
	removePrevModelSet();

    if (modelSet.isNull()) return;
    m_prevModelSet = modelSet;

    ItemBase * keyItem = modelSet->keyItem();
    if (keyItem == NULL) {
        long id = modelSet->keyId();
        keyItem = findItem(id);
    }
    if (keyItem != NULL) {
        modelSet->setKeyItem(keyItem);
        keyItem->setModelSet(modelSet);
    }

    QList<ModelSet::TerminalPair> connectionList = modelSet->getConnections();
    foreach(ModelSet::TerminalPair c, connectionList) {
        ModelSet::Terminal from = c.first;
        ModelSet::Terminal to = c.second;
        ItemBase * fromItem = modelSet->getItem(modelSet->genLabelHashKey(from));
        if (fromItem == NULL) {
            fromItem = addSetItem(QPointF(0, 0), from.moduleID, transparent);
            if (fromItem == NULL) {
                continue;
            }
            modelSet->insertLabelHash(modelSet->genLabelHashKey(from), fromItem);
            modelSet->addItem(fromItem);
            fromItem->setModelSet(modelSet);
        }
        if (to.title == "NULL" || to.title == "") continue;
        ItemBase * toItem = modelSet->getItem(modelSet->genLabelHashKey(to));
        if (toItem == NULL) {
            toItem = addSetItem(fromItem, from.connectorID, to.moduleID, to.connectorID, transparent);
            if (toItem == NULL) {
                continue;
        	}
            modelSet->insertLabelHash(modelSet->genLabelHashKey(to), toItem);
            modelSet->addItem(toItem);
            toItem->setModelSet(modelSet);
           
        }

        DebugDialog::debug(QString("from: %1, to: %2").arg(fromItem->title()).arg(toItem->title()));
        ItemBase * wire = addSetWire(fromItem, from.connectorID, toItem, to.connectorID, transparent);
        if (wire == NULL) continue;
        wire->setModelSet(modelSet);
        //Wire * w = qobject_cast<Wire *>(wire);
        modelSet->addItem(wire);
        //TODO: arduino
    }
    if (!transparent) confirmSelect(modelSet);

}

void SketchWidget::removePrevModelSet() {

    if (m_prevModelSet.isNull()) return;
    QList<ItemBase *> itemList = m_prevModelSet->getItemList();
    QSharedPointer<SetConnection> setConnection = m_prevModelSet->setConnection();
    foreach(ItemBase * itemBase, itemList) {
        //itemBase->removeLayerKin();
        this->scene()->removeItem(itemBase);
        if (itemBase->modelPart()) {
            delete itemBase->modelPart();
        }
        delete itemBase;
	}
    removePrevSetConnection();
	m_prevModelSet->emptyItemList();
    m_prevModelSet.clear();
}

void SketchWidget::removePrevSetConnection() {
    if (m_prevModelSet.isNull()) return;
    QSharedPointer<SetConnection> setConnection = m_prevModelSet->setConnection();
    if (setConnection.isNull()) return;
    QList<ItemBase *> itemList = setConnection->getWireList();

    foreach(ItemBase * itemBase, itemList) {
        //itemBase->removeLayerKin();
        this->scene()->removeItem(itemBase);
        if (itemBase->modelPart()) {
            delete itemBase->modelPart();
        }
        delete itemBase;
    }
    setConnection->emptyWireList();
    m_prevModelSet->clearSetConnection();

}

ItemBase * SketchWidget::addSetWire(ItemBase * fromItem, const QString & fromConnectorID, ItemBase * toItem, const QString & toConnectorID, bool transparent) {
    //DebugDialog::debug(QString("%1").arg(fromItem->title()));
    // ItemBase * itemBase = findItem(fromItem->id());
    // if (itemBase == NULL) {
    //     return NULL;
    // }

    ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, fromItem->viewLayerPlacement());
    if (fromConnectorItem == NULL) return NULL;

	ModelPart * wirePart = m_referenceModel->retrieveModelPart(QString("WireModuleID"));
	long wireID = ItemBase::getNextID();
	ViewGeometry viewGeometryWire;
	QLineF line;
    //QList<ConnectorItem *> exclude;
    //ConnectorItem * fromOverConnectorItem = fromConnectorItem->findConnectorUnder(true, true, exclude, true, fromConnectorItem);
    //QPointF fromOverPos = fromOverConnectorItem->sceneAdjustedTerminalPoint(NULL);
    QPointF fromConnectorPos = fromConnectorItem->sceneAdjustedTerminalPoint(NULL);
    ConnectorItem * toConnectorItem = findConnectorItem(toItem, toConnectorID, toItem->viewLayerPlacement());
	QPointF toConnectorPos = toConnectorItem->sceneAdjustedTerminalPoint(NULL);

    line.setLine(0, 0, -fromConnectorPos.x()+toConnectorPos.x(), -fromConnectorPos.y()+toConnectorPos.y());

    viewGeometryWire.setLoc(fromConnectorPos);
	viewGeometryWire.setLine(line);
    ItemBase * wire = addItemAuxTemp(wirePart, defaultViewLayerPlacement(wirePart), viewGeometryWire, wireID, true, m_viewID, true);
   	if (wire == NULL) return NULL;
    if (transparent) wire->setOpacity(0.5);

    changeConnection(fromItem->id(), fromConnectorID, wireID, "connector0", ViewLayer::specFromID(fromConnectorItem->attachedToViewLayerID()), true, false, false);
	changeConnection(toItem->id(), toConnectorID, wireID, "connector1",ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()), true, false, false);
    return wire;
}

ItemBase * SketchWidget::addSetItem(QPointF pos, QString & toModuleID, bool transparent) {

	toModuleID = checkDroppedModuleID(toModuleID);
	ModelPart * modelPart = m_referenceModel->retrieveModelPart(toModuleID);
    if (modelPart ==  NULL) return NULL;
    if (!canDropModelPart(modelPart)) return NULL;


	ViewGeometry viewGeometry;
    //QPointF fromOverPos = connectorItem->sceneAdjustedTerminalPoint(NULL);

	long toID = ItemBase::getNextID();
	bool doConnectors = true;
	ItemBase *toItem = addItemAuxTemp(modelPart, defaultViewLayerPlacement(modelPart), viewGeometry, toID, doConnectors, m_viewID, false);
    toItem->setPos(pos);

    if (transparent) toItem->setOpacity(0.5);

    return toItem;

}


ItemBase * SketchWidget::addSetItem(ItemBase * fromItem, const QString & fromConnectorID, QString toModuleID, const QString & toConnectorID, bool transparent) { // TODO: add offset, paras
	
	//toModuleID = QString("ResistorModuleID");

    //QString toConnectorID2 = QString("connector1");
	// get from item
    ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, fromItem->viewLayerPlacement());
    if (fromConnectorItem == NULL) return NULL; // for now

	//check connection item under
	QList<ConnectorItem *> exclude;
    ConnectorItem * fromOverConnectorItem = fromConnectorItem->findConnectorUnder(true, true, exclude, true, fromConnectorItem);
    ConnectorItem * connectorItem = fromOverConnectorItem == NULL ? fromConnectorItem : fromOverConnectorItem;
    //TODO fromOverConnectorItem == NULL?
    if (connectorItem != NULL) {
        //DebugDialog::debug(QString("fromOverConnectorItem:%1").arg(fromOverConnectorItem->connectorSharedID()));
        //fromOverConnectorItem->setOverConnectorItem(NULL);   // clean up
		toModuleID = checkDroppedModuleID(toModuleID);
		ModelPart * modelPart = m_referenceModel->retrieveModelPart(toModuleID);
        if (modelPart ==  NULL) return NULL;
        if (!canDropModelPart(modelPart)) return NULL;


		ViewGeometry viewGeometry;
        QPointF fromOverPos = connectorItem->sceneAdjustedTerminalPoint(NULL);

		long toID = ItemBase::getNextID();
		bool doConnectors = true;
		ItemBase *toItem = addItemAuxTemp(modelPart, defaultViewLayerPlacement(modelPart), viewGeometry, toID, doConnectors, m_viewID, true);
        
        ConnectorItem * toConnectorItem = findConnectorItem(toItem, toConnectorID, toItem->viewLayerPlacement());
		QPointF toPos = toItem->getViewGeometry().loc();
		QPointF toConnectorPos = toConnectorItem->sceneAdjustedTerminalPoint(NULL);

		if (modelPart->family() != "microcontroller board (arduino)") {
		//if (toModuleID != "arduino_Uno_Rev3(fix)") {
			QPointF offset(0, 0);
            QPointF terminalOffset = toPos-toConnectorPos;
            QPointF fromCenter = fromItem->getViewGeometry().loc()+fromItem->boundingRect().center();
            if (fromCenter.x()- fromOverPos.x() > 0) {
				offset.setX(-36);
			} else {
				offset.setX(36);
			}

			toItem->setPos(fromOverPos+terminalOffset+offset);
        }
        if (transparent) toItem->setOpacity(0.5);

        //changeConnection(fromOverConnectorItem->attachedTo()->id(), fromOverConnectorItem->connectorSharedID(), wireID, "connector0", ViewLayer::specFromID(fromOverConnectorItem->attachedToViewLayerID()), true, false, false);
        //changeConnection(toID, toConnectorID, wireID, "connector1",ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()), true, false, false);
		//extendChangeConnectionCommand(BaseCommand::CrossView, toConnectorItem, wire->connector0, ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()), true, parentCommand);
        return toItem;
	}
    return NULL;

}

void SketchWidget::checkMousePressSuggestion(QGraphicsItem * item) {

    ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
    if (itemBase) {
        QSharedPointer<ModelSet> modelset = itemBase->modelSet();
        if (!modelset.isNull()) {
            m_pressModelSet = modelset;

        }
    }
}

void SketchWidget::checkSelectSuggestion() {
    if (!m_autoComplete || m_pressModelSet.isNull()) return;
    setOpacity(m_pressModelSet);
    confirmSelect(m_pressModelSet);
}

void SketchWidget::confirmSelect(QSharedPointer<ModelSet> modelSet) {

    m_prevModelSet.reset();
    m_pressModelSet.reset();
    m_savedModelSet.append(modelSet);
    /*
    if (modelSet->keyId() != -1) {
        ItemBase * keyItem = findItem(modelSet->keyId());
        if (keyItem != NULL) modelSet->setKeyItem(keyItem);
    }*/

    AutoCompleter::getSuggestionNext(modelSet, this);

    return;
}

void SketchWidget::setOpacity(QSharedPointer<ModelSet> modelSet) {
    if (modelSet.isNull()) return;

    QList<ItemBase *> itemList = modelSet->getItemList();
    if (itemList.length() == 0) return;
    foreach(ItemBase * item, itemList) {
        if (item->opacity() == 1) return;
        item->setOpacity(1);
    }
    setOpacity(modelSet->setConnection());
}

void SketchWidget::setOpacity(QSharedPointer<SetConnection> setConnection) {
    if (setConnection.isNull()) return;

    QList<ItemBase *> itemList = setConnection->getWireList();
    if (itemList.length() == 0) return;
    foreach(ItemBase * item, itemList) {
        if (item->opacity() == 1) return;
        item->setOpacity(1);
    }
}
