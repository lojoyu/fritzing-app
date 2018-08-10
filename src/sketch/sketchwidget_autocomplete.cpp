
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

void SketchWidget::setAutoComplete(bool autoComplete) {
	m_autoComplete = autoComplete;
}

void SketchWidget::addSetToSet(ModelSet * modelSet, SetConnection * setconnection, bool transparent) {
    addModelSet(modelSet, transparent);
    addSetConnection(setconnection, transparent);
}

void SketchWidget::addSetConnection(SetConnection * setconnection, bool transparent) {
	ModelSet * from = setconnection->getFromModelSet();
	ModelSet * to = setconnection->getToModelSet();

	QList<QPair<long, long>> connectionList = setconnection->getConnectionList();
    foreach(LongLongPair c, connectionList) {
		QPair<ItemBase *, QString> p1 = from->getItemAndCID(c.first);
		QPair<ItemBase *, QString> p2 = to->getItemAndCID(c.second);
        if (p1.first == NULL || p2.first == NULL) continue;
        ItemBase * wire = addSetWire(p1.first, p1.second, p2.first, p2.second, transparent);
		setconnection->appendWireList(wire);
	}
	
}

void SketchWidget::addModelSet(ModelSet * modelSet, bool transparent) {

	if (modelSet == m_prevModelSet) return;
	removePrevModelSet();

    ItemBase * keyItem = modelSet->keyItem();
    if (keyItem != NULL) {
        keyItem->setModelSet(modelSet);
    }
    QList<ModelSet::TerminalPair> connectionList = modelSet->getConnections();
    foreach(ModelSet::TerminalPair c, connectionList) {
        ModelSet::Terminal from = c.first;
        ModelSet::Terminal to = c.second;
        ItemBase * fromItem = modelSet->getItem(QString("%1%2").arg(from.title).arg(from.label));
        if (fromItem == NULL) {

            fromItem = addSetItem(QPointF(0, 0), from.moduleID, transparent);
            if (fromItem == NULL) {
                continue;
            }
            modelSet->insertLabelHash(modelSet->genLabelHashKey(from), fromItem);
            modelSet->addItem(fromItem);
            fromItem->setModelSet(modelSet);
        }
        ItemBase * toItem = modelSet->getItem(QString("%1%2").arg(to.title).arg(to.label));
        if (toItem == NULL) {
            toItem = addSetItem(fromItem, from.connectorID, to.moduleID, to.connectorID, transparent);
        	if (toItem == NULL) {
                continue;
        	}
            DebugDialog::debug(QString("%1%2").arg(to.title).arg(to.label));
            modelSet->insertLabelHash(modelSet->genLabelHashKey(to), toItem);
            modelSet->addItem(toItem);
            toItem->setModelSet(modelSet);
        }
        ItemBase * wire = addSetWire(fromItem, from.connectorID, toItem, to.connectorID, transparent);
        wire->setModelSet(modelSet);
        //Wire * w = qobject_cast<Wire *>(wire);
        modelSet->addItem(wire);

        //TODO: arduino
    }
    m_prevModelSet = modelSet;
}

void SketchWidget::removePrevModelSet() {
	if (m_prevModelSet == NULL) return;
    QList<ItemBase *> itemList = m_prevModelSet->getItemList();

    foreach(ItemBase * itemBase, itemList) {
        //itemBase->removeLayerKin();
		this->scene()->removeItem(itemBase);
        if (itemBase->modelPart()) {
            delete itemBase->modelPart();
        }
		delete itemBase;
	}
	m_prevModelSet->emptyItemList();
}

ItemBase * SketchWidget::addSetWire(ItemBase * fromItem, const QString & fromConnectorID, ItemBase * toItem, const QString & toConnectorID, bool transparent) {
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
	ItemBase *toItem = addItemAuxTemp(modelPart, defaultViewLayerPlacement(modelPart), viewGeometry, toID, doConnectors, m_viewID, true);
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
        ModelSet * modelset = itemBase->modelSet();
        if (modelset != NULL) {
            m_pressModelSet = modelset;
        }
    }
}

void SketchWidget::checkSelectSuggestion() {
    if (!m_autoComplete || m_pressModelSet == NULL) return;
    QList<ItemBase *> itemList = m_pressModelSet->getItemList();
    foreach(ItemBase * item, itemList) {
        if (item->opacity() == 1) return;
        item->setOpacity(1);
    }
    m_prevModelSet = NULL;
    AutoCompleter::getSuggestionNext(m_pressModelSet, this);
    m_pressModelSet = NULL;
}

