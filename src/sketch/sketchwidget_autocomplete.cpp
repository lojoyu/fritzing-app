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
typedef QPair<ModelSet::Terminal, QString> TerminalStringPair;

void SketchWidget::setAutoComplete(bool autoComplete) {
	m_autoComplete = autoComplete;
    autocompleteInit();
}

void SketchWidget::autocompleteInit() {
    if (!m_autoComplete) return;
    if (m_addedDefaultPart.isNull()) return;
    QString title = m_addedDefaultPart->title();
    QString moduleID = m_addedDefaultPart->moduleID();
    QString label = m_addedDefaultPart->label();
    m_breadBoardModelSet = QSharedPointer<ModelSet>(new ModelSet(-1, moduleID));
    m_breadBoardModelSet->setKeyLabel(label);
    m_breadBoardModelSet->setKeyItem(m_addedDefaultPart);
    foreach(ConnectorItem * connectorItem, m_addedDefaultPart->cachedConnectorItems()) {
        DebugDialog::debug(QString("---%1").arg(connectorItem->connectorSharedID()));
        ModelSet::Terminal t1 = ModelSet::Terminal(moduleID, title, label, connectorItem->connectorSharedID(), "");
        ModelSet::Terminal t2 = ModelSet::Terminal("NULL", "NULL", "NULL", "NULL", "");
        m_breadBoardModelSet->appendConnection(t1, t2);
        m_breadBoardModelSet->insertTerminalnameHash(connectorItem->connectorSharedID(), t1);
    }
}

void SketchWidget::selectModelSet(QSharedPointer<ModelSet> modelSet, bool transparent) {
    if (transparent) addToModelSet(modelSet, transparent);
    else {
        QUndoCommand* parentCommand = new TemporaryCommand(tr("Add modelSet %1").arg(modelSet->keyTitle()));
        newAddModelSetCommand(modelSet, parentCommand);
        m_undoStack->waitPush(parentCommand, 10);
    }
}

void SketchWidget::selectSetToSet(QSharedPointer<ModelSet> modelSet, QSharedPointer<SetConnection> setconnection, bool confirmSetConnection, bool transparent) {
    if (transparent) addSetToSet(modelSet, setconnection, confirmSetConnection, transparent);
    else if (modelSet->isConfirm()) {
        newAddSetConnectionCommand(setconnection, NULL);
    } else {
        QUndoCommand* parentCommand = new TemporaryCommand(tr("Add modelSet / Connection %1").arg(modelSet->keyTitle()));
        newAddSetToSetCommand(modelSet, setconnection, confirmSetConnection, parentCommand);
        m_undoStack->waitPush(parentCommand, 10);
    }
}

AddModelSetCommand * SketchWidget::newAddModelSetCommand(QSharedPointer<ModelSet> modelSet, QUndoCommand * parentCommand) {
    return new AddModelSetCommand(this, modelSet, parentCommand);
}

AddSetToSetCommand * SketchWidget::newAddSetToSetCommand(QSharedPointer<ModelSet> modelSet, QSharedPointer<SetConnection> setConnection, bool confirmSetConnection, QUndoCommand * parentCommand) {
    return new AddSetToSetCommand(this, modelSet, setConnection, confirmSetConnection, parentCommand);
}

AddSetConnectionCommand * SketchWidget::newAddSetConnectionCommand(QSharedPointer<SetConnection> setConnection, QUndoCommand *parentCommand) {
    bool newParent = false;
    if (parentCommand == NULL) {
        QSharedPointer<ModelSet> from = setConnection->getFromModelSet();
        QSharedPointer<ModelSet> to = setConnection->getToModelSet();
        parentCommand = new TemporaryCommand(tr("Add connection between %1, %2").arg(from->keyTitle()).arg(to->keyTitle()));
        newParent = true;
    }
    AddSetConnectionCommand * addSetConnectionCommand = new AddSetConnectionCommand(this, setConnection, parentCommand);
    if (newParent) m_undoStack->waitPush(parentCommand, 10);

    return addSetConnectionCommand;
}

void SketchWidget::addToModelSet(QSharedPointer<ModelSet> modelSet, bool transparent) {
    findKeyItem(modelSet);
    
    addModelSet(modelSet, transparent);
    addSetConnection(modelSet->breadboardConnection(), transparent);
    QSharedPointer<SetConnection> breadBoardSetConnection = modelSet->breadboardConnection();
    if (!breadBoardSetConnection.isNull())
        modelSet->addBreadboardConnection(breadBoardSetConnection);

    if (!transparent || (modelSet->single() && breadBoardSetConnection.isNull())) {
        setOpacity(modelSet);
        setOpacity(modelSet->breadboardConnection());
        confirmSelect(modelSet);
    }
}

void SketchWidget::addSetToSet(QSharedPointer<ModelSet> modelSet, QSharedPointer<SetConnection> setconnection, bool confirmSetConnection, bool transparent) {
    QSharedPointer<ModelSet> temp = m_prevModelSet;
    QSharedPointer<ModelSet> from = setconnection->getFromModelSet();
    findKeyItem(from);
    if (from->keyItem()!=NULL) {
        m_tempPoint = from->keyItem()->getViewGeometry().loc()+from->keyItem()->boundingRect().center();
        m_tempPoint = m_tempPoint-QPoint(90, 0);
    }

    addModelSet(modelSet, transparent);
    addSetConnection(modelSet->breadboardConnection(), true);
    //if (transparent || temp!= modelSet)
    addSetConnection(setconnection, true);
    if (!modelSet->breadboardConnection().isNull())
        modelSet->addBreadboardConnection(modelSet->breadboardConnection());
    modelSet->addSetConnection(setconnection);
    if (!transparent) {
        setOpacity(modelSet);
        //setOpacity(modelSet->setConnection());
        confirmSelect(modelSet);
    }

}

void SketchWidget::addSetConnection(QSharedPointer<SetConnection> setconnection, bool transparent) {
    if (setconnection.isNull() || setconnection->isConfirm()) return;

    if (setconnection == m_prevModelSet->setConnection() || setconnection == m_prevModelSet->breadboardConnection()) {
        if (!transparent) {
            setOpacity(setconnection);
            //confirmSelect(modelSet);
        }
        return;
    }
    removePrevSetConnection(false);
    QSharedPointer<ModelSet> from = setconnection->getFromModelSet();
    QSharedPointer<ModelSet> to = setconnection->getToModelSet();
    if (from.isNull() || to.isNull()) return;
    //findKeyItem(from);
    QList<QString> usedConnectorID;

	QList<SetConnection::Connection> connectionList = setconnection->getConnectionList();
    foreach(SetConnection::Connection c, connectionList) {
        QPair<ItemBase *, QString> p1 = from->getItemAndCID(c.fromTerminal);
        QPair<ItemBase *, QString> p2;
        QList<QPair<ModelSet::Terminal, QString>> pintypeT = to->getPinTypeTerminal(c.toTerminal);
        QList<QPair<ModelSet::Terminal, QString>> breadboardT = m_breadBoardModelSet->getPinTypeTerminal(c.toTerminal);
        if (to->isMicrocontroller() && breadboardT.length() > 0 && from != m_breadBoardModelSet) {
            ModelSet::Terminal t = breadboardT[0].first;
            QString vccConnectorID = t.connectorID;
            QList<QString> vccConnectorIDList;
            vccConnectorIDList.append(vccConnectorID);
            vccConnectorID = findBreadBoardUnused(vccConnectorIDList, vccConnectorIDList, false);
            p2 = QPair<ItemBase *, QString>(m_addedDefaultPart, vccConnectorID);
        } else if (to->isMicrocontroller() && pintypeT.length() > 0 && from != m_breadBoardModelSet) {

            QString connectorID = pintypeT[0].first.connectorID;
            foreach(TerminalStringPair tpair, pintypeT) {
                if (usedConnectorID.contains(tpair.first.connectorID)) continue;
                usedConnectorID.append(tpair.first.connectorID);
                ConnectorItem * connectorItem = to->keyItem()->findConnectorItemWithSharedID(tpair.first.connectorID);
                QList<QPointer<ConnectorItem>> connectorItemList = connectorItem->connectedToItems();
                if (connectorItemList.length() != 0) continue;
                else {
                    connectorID = tpair.first.connectorID;
                    break;
                }
            }
            p2 = QPair<ItemBase *, QString>(to->keyItem(), connectorID);

        } else {
            p2 = to->getItemAndCID(c.toTerminal);
        }

        if (p1.first == NULL|| p2.first == NULL || p1.second == "" ||  p2.second == "") continue;
        DebugDialog::debug(QString("p1: %1").arg(p1.first->title()));
        DebugDialog::debug(QString("p2: %1").arg(p2.first->title()));
        ItemBase * wire = addSetWire(p1.first, p1.second, p2.first, p2.second, transparent);
        if (wire == NULL) continue;
        if (c.changeColor) {
            Wire * w = qobject_cast<Wire *>(wire);
            w->setColor(c.color, 1);
        }
        //p1.first(item) -> id?, p1.second
        wire->setModelSet(to);
        setconnection->insertWireConnection(wire, p1.first->id(), p1.second, p2.first->id(), p2.second);
		setconnection->appendWireList(wire);
        //TODO: wire store which set connection it belongs to
	}

}

void SketchWidget::addModelSet(QSharedPointer<ModelSet> modelSet, bool transparent) {

    //findKeyItem(modelSet);
    m_savedModelSet.append(modelSet);
    if (modelSet == m_prevModelSet) {
       //removePrevSetConnection(false);
       if (!transparent) {
           setOpacity(modelSet);
           setOpacity(modelSet->breadboardConnection());
           //confirmSelect(modelSet);
       }
       return;
    }
	removePrevModelSet();

    if (modelSet.isNull()) return;
    m_prevModelSet = modelSet;


    QList<ModelSet::TerminalPair> connectionList = modelSet->getConnections();
    foreach(ModelSet::TerminalPair c, connectionList) {
        ModelSet::Terminal from = c.first;
        ModelSet::Terminal to = c.second;
        ItemBase * fromItem = modelSet->getItem(modelSet->genLabelHashKey(from));
        if (fromItem == NULL) {
            if (from.title == "Arduino Uno (Rev3)") m_tempPoint = QPointF(100, 300);
            fromItem = addSetItem(m_tempPoint, from.moduleID, transparent);

            if (fromItem == NULL) {
                continue;
            }
            if (from.label == modelSet->keyLabel()){
                modelSet->setKeyId(fromItem->id());
                modelSet->setKeyItem(fromItem);
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
            if (to.label == modelSet->keyLabel()){
                modelSet->setKeyId(toItem->id());
                modelSet->setKeyItem(toItem);
            }
            modelSet->insertLabelHash(modelSet->genLabelHashKey(to), toItem);
            modelSet->addItem(toItem);
            toItem->setModelSet(modelSet);
           
        }

        DebugDialog::debug(QString("from: %1, to: %2").arg(fromItem->title()).arg(toItem->title()));
        ItemBase * wire = addSetWire(fromItem, from.connectorID, toItem, to.connectorID, transparent);
        if (wire == NULL) continue;
        wire->setModelSet(modelSet);
        modelSet->insertWireConnection(wire, QPair<ModelSet::Terminal, ModelSet::Terminal>(from, to));
        //Wire * w = qobject_cast<Wire *>(wire);
        modelSet->addItem(wire);
        //TODO: arduino
    }
    completeSuggestion(modelSet, transparent);
    //if (!transparent) confirmSelect(modelSet);

}

void SketchWidget::findKeyItem(QSharedPointer<ModelSet> modelSet) {
    //TODO: change to command type
    if (modelSet.isNull()) return;
    ItemBase * keyItem = modelSet->keyItem();
    long id = modelSet->keyId();
    keyItem = findItem(id);
    if (keyItem != NULL) {
        modelSet->setKeyItem(keyItem);
        keyItem->setModelSet(modelSet);
    }

}

void SketchWidget::deleteModelSet(QSharedPointer<ModelSet> modelSet) {
    if (modelSet.isNull()) return;
    QList<ItemBase *> itemList = modelSet->getItemList();
    foreach(ItemBase * itemBase, itemList) {
        deleteItem(itemBase, true, false, false);
    }
    modelSet->emptyItemList();
    modelSet->setConfirm(false);
    if (modelSet->keyItem() != NULL) {
        modelSet->keyItem()->setModelSet(QSharedPointer<ModelSet>(NULL));
    }

    // empty list
    deleteSetConnection(modelSet->setConnection());
    deleteSetConnection(modelSet->breadboardConnection());
    if (modelSet != m_prevModelSet && !m_prevModelSet.isNull()) {
        deleteModelSet(m_prevModelSet);
        m_prevModelSet.clear();
    }
    if (m_savedModelSet.contains(modelSet)) {
        m_savedModelSet.removeOne(modelSet);
    }
    AutoCompleter::clearRecommend();
}

void SketchWidget::deleteSetConnection(QSharedPointer<SetConnection> setConnection) {
    if (setConnection.isNull()) return;
    QList<ItemBase *> itemList;
    itemList.append(setConnection->getWireList());
    foreach(ItemBase * itemBase, itemList) {
        deleteItem(itemBase, false, false, false);

    }
    setConnection->emptyWireList();
    if (setConnection->isConfirm()) {
        setConnection->setConfirm();
    }
    QSharedPointer<ModelSet> fromModelSet = setConnection->getFromModelSet();
    QSharedPointer<ModelSet> toModelSet = setConnection->getToModelSet();
    fromModelSet->deleteSetConnection(setConnection);
    toModelSet->deleteSetConnection(setConnection);

}

void SketchWidget::removePrevModelSet() {

    if (m_prevModelSet.isNull()) return;
    if (!m_prevModelSet->isConfirm()) {
        QList<ItemBase *> itemList = m_prevModelSet->getItemList();
        foreach(ItemBase * itemBase, itemList) {
            if (itemBase == m_prevModelSet->keyItem()) {
                m_prevModelSet->setKeyItem(NULL);
            }
            deleteItem(itemBase, false, false, false);

//            //itemBase->removeLayerKin();
//            this->scene()->removeItem(itemBase);
//            if (itemBase->modelPart()) {
//                delete itemBase->modelPart();
//            }
//            delete itemBase;
        }
    }
    removePrevSetConnection(true);
    //TODO: m_prevModelSet Key Item delete?
	m_prevModelSet->emptyItemList();
    m_prevModelSet.clear();

}

void SketchWidget::removePrevSetConnection(bool removeBreadboard) {
    if (m_prevModelSet.isNull()) return;
    QSharedPointer<SetConnection> setConnection = m_prevModelSet->setConnection();
    QSharedPointer<SetConnection> breadboardConnection = m_prevModelSet->breadboardConnection();
    if (setConnection.isNull() && breadboardConnection.isNull()) return;
    QList<ItemBase *> itemList;
    if (!setConnection.isNull() && !setConnection->isConfirm()) itemList.append(setConnection->getWireList());
    if (!breadboardConnection.isNull() && removeBreadboard && !breadboardConnection->isConfirm()) itemList.append(breadboardConnection->getWireList());

    foreach(ItemBase * itemBase, itemList) {
        //itemBase->removeLayerKin();
        deleteItem(itemBase, false, false, false);

//        this->scene()->removeItem(itemBase);
//        if (itemBase->modelPart()) {
//            delete itemBase->modelPart();
//        }
//        delete itemBase;
    }
    if (!setConnection.isNull()) setConnection->emptyWireList();
    if (!breadboardConnection.isNull() && removeBreadboard && !breadboardConnection->isConfirm()) breadboardConnection->emptyWireList();
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
    ItemBase * wire = addItemAuxTemp(wirePart, defaultViewLayerPlacement(wirePart), viewGeometryWire, wireID, true, m_viewID, transparent);
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
    ItemBase *toItem = addItem(toModuleID, defaultViewLayerPlacement(modelPart), BaseCommand::CrossView, viewGeometry, toID, -1, NULL);
            //addItemAuxTemp(modelPart, defaultViewLayerPlacement(modelPart), viewGeometry, toID, doConnectors, m_viewID, transparent);

    //ItemBase *toItem = addItemAuxTemp(modelPart, defaultViewLayerPlacement(modelPart), viewGeometry, toID, doConnectors, m_viewID, transparent);

    toItem->setPos(pos-toItem->boundingRect().center());

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
                offset.setX(-27);
			} else {
                offset.setX(27);
			}
            offset.setY(9);
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

void SketchWidget::getSuggestionConnection(Wire *wire, ConnectorItem * to) {
    if (!m_autoComplete) return;
    if (to == NULL) return;

    ConnectorItem * toConnectorItem = NULL;
    if (to->attachedTo() != m_breadBoardModelSet->keyItem()) {
        toConnectorItem = to;
    } else {
        QList<ConnectorItem *> connectorItems;
        connectorItems.append(to);
        ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);
        foreach(ConnectorItem * citem, connectorItems) {
            ItemBase * item = citem->attachedTo();
            QString s = citem->attachedTo()->title();
            if (item != m_breadBoardModelSet->keyItem() && item != wire) {
                toConnectorItem = citem;
                //break;
            }
        }
    }

    ConnectorItem * connector0 = wire->connector0();
    ConnectorItem * fromConnectorItem = findConnectorItemTo(connector0, toConnectorItem);
    //ConnectorItem * connector1 = wire->connector1();
    //ConnectorItem * toConnectorItem = findConnectorItemTo(connector1, NULL);
    if (fromConnectorItem == NULL || toConnectorItem == NULL) return;


    DebugDialog::debug(QString("%1 attached to : %2").arg(fromConnectorItem->connectorSharedID()).arg(fromConnectorItem->attachedTo()->title()));
    DebugDialog::debug(QString("attached to : %1").arg(toConnectorItem->attachedTo()->title()));

    foreach (QSharedPointer<ModelSet> ms, m_savedModelSet) {
        findKeyItem(ms);
    }

    QSharedPointer<ModelSet> fromModelSet = fromConnectorItem->attachedTo()->modelSet();
    QSharedPointer<ModelSet> toModelSet = toConnectorItem->attachedTo()->modelSet();
    if (fromModelSet.isNull() || toModelSet.isNull()) return;

    AutoCompleter::getSuggestionConnection(fromModelSet, fromConnectorItem, toModelSet, toConnectorItem, this);

}

ConnectorItem * SketchWidget::findConnectorItemTo(ConnectorItem * connectorItem, ConnectorItem * excludeConnector){
    ConnectorItem * fromConnectorItem = NULL;
    QList<ConnectorItem *> exclude;
    if (excludeConnector != NULL) exclude.append(excludeConnector);
    ConnectorItem * under = connectorItem->findConnectorUnder(true, true, exclude, false, NULL);
    if (under == NULL) return NULL;

    if (under->attachedTo() == m_breadBoardModelSet->keyItem()) {
        QList<ConnectorItem *> connectorItems;
        connectorItems.append(connectorItem);
        ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);
        foreach(ConnectorItem * citem, connectorItems) {
            ItemBase * item = citem->attachedTo();
            QString s = citem->attachedTo()->title();
            if (item != m_breadBoardModelSet->keyItem() && item != connectorItem->attachedTo()) {
                fromConnectorItem = citem;
                //break;
            }
        }
    } else {
        fromConnectorItem = under;
    }
    return fromConnectorItem;

}

QSharedPointer<ModelSet> SketchWidget::getMicrocontroller() {
    foreach(QSharedPointer<ModelSet> modelset, m_savedModelSet) {
        if (modelset->isMicrocontroller()) return modelset;
    }
    return QSharedPointer<ModelSet>(NULL);
}

QString SketchWidget::findBreadBoardUnused(QList<QString> connectorIDList, QList<QString> excludeConnectorIDList, bool checkConnected) {
    if (m_breadBoardModelSet.isNull()) return "";
    
    ItemBase * breadboard = m_breadBoardModelSet->keyItem();
    foreach(QString s, connectorIDList) {

        ConnectorItem * connectorItem = breadboard->findConnectorItemWithSharedID(s);
        QList<QPointer<ConnectorItem>> connectorItemList = connectorItem->connectedToItems();
        if (connectorItemList.length() != 0 && checkConnected) continue;

        bool connected = false;
        QList<ConnectorItem *> connectorItems;
        connectorItems.append(connectorItem);
        ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);
        foreach(ConnectorItem * citem, connectorItems) {
            if (excludeConnectorIDList.contains(citem->connectorSharedID())) continue;
            connectorItemList = citem->connectedToItems();
            if (connectorItemList.length() == 0) return citem->connectorSharedID();
            if (connectorItemList.length() != 0 && checkConnected) {
                connected = true;
                break;
            }
        }
        if (!connected && checkConnected) return s;

    }
    return "";
}

void SketchWidget::completeSuggestion(QSharedPointer<ModelSet> modelset, bool transparent) {
    QList<QPair<ModelSet::Terminal, QString>> vccT = modelset->getPinTypeTerminal("VCC");
    QList<QPair<ModelSet::Terminal, QString>> gndT = modelset->getPinTypeTerminal("GND");
    QSharedPointer<SetConnection> setconnection;
    //if m_addedDefaultPart != NLL
    if (modelset->isMicrocontroller()) {
        if ((modelset->breadboardConnection()).isNull()) {
            setconnection = QSharedPointer<SetConnection>(new SetConnection(m_breadBoardModelSet, modelset));

            QList<QString> vccConnectorIDList({"pin3W", "pin3Y"});
            QList<QString> gndConnectorIDList({"pin3X", "pin3Z"});
            QString vccConnectorID = findBreadBoardUnused(vccConnectorIDList, QList<QString>(), true);
            QString gndConnectorID = findBreadBoardUnused(gndConnectorIDList, QList<QString>(), true);
            if (vccT.length() > 0 && vccConnectorID!="") {
                TerminalStringPair vccPair = vccT[0];
                foreach(TerminalStringPair pair, vccT) {
                    if (pair.second == "5V") {
                        vccPair = pair;
                        break;
                    }
                }
                //setconnection->appendConnection(vccPair.first.connectorID, vccConnectorID, QColor(255, 0, 0));
                setconnection->appendConnection(vccConnectorID, vccPair.second, QColor(255, 0, 0));
                m_breadBoardModelSet->insertTerminalType(vccConnectorID, "VCC");
                m_breadBoardModelSet->insertTerminalType(vccConnectorID, vccPair.second);
            }
            if (gndT.length() > 0 && gndConnectorID!="") {
                setconnection->appendConnection(gndConnectorID, gndT[0].second,  QColor(0, 0, 0));
                //setconnection->appendConnection(gndT[0].first.connectorID, gndConnectorID, QColor(0, 0, 0));
                m_breadBoardModelSet->insertTerminalType(gndConnectorID, gndT[0].second);
            }
            modelset->addBreadboardConnection(setconnection);
        }

    }
//    if (!setconnection.isNull()) {
//        addSetConnection(setconnection, transparent);
//        modelset->addSetConnection(setconnection);
//        if (!transparent) setconnection->setConfirm();
//    }

}

/*
void SketchWidget::completeSuggestion(QSharedPointer<ModelSet> modelset, bool transparent) {
    

    QList<QPair<ModelSet::Terminal, QString>> vccT = modelset->getPinTypeTerminal("VCC");
    QList<QPair<ModelSet::Terminal, QString>> gndT = modelset->getPinTypeTerminal("GND");

    //if m_addedDefaultPart != NLL
    if (modelset->isMicrocontroller()) {

        //TODO: change --> save microcontroller field in db
        QList<QString> vccConnectorIDList({"pin3W", "pin3Y"});
        QList<QString> gndConnectorIDList({"pin3X", "pin3Z"});
        QString vccConnectorID = findBreadBoardUnused(vccConnectorIDList, QList<QString>(), true);
        QString gndConnectorID = findBreadBoardUnused(gndConnectorIDList, QList<QString>(), true);
        DebugDialog::debug("Add Microcontroller!");
        if (vccT.length() > 0 && vccConnectorID!="") {
            TerminalStringPair vccPair = vccT[0];
            foreach(TerminalStringPair pair, vccT) {
                if (pair.second == "5V") {
                    vccPair = pair;
                    break;
                }
            }
            ItemBase * wire = addSetWire(modelset->getItem(vccPair.first.label), vccPair.first.connectorID, m_addedDefaultPart, vccConnectorID, transparent);
            Wire * w = qobject_cast<Wire *>(wire);
            w->setColor(QColor(255, 0, 0), 1);
            m_breadBoardModelSet->insertTerminalType("VCC", vccConnectorID);
            m_breadBoardModelSet->insertTerminalType(vccPair.second, vccConnectorID);

        }
        if (gndT.length() > 0 && gndConnectorID!="") {
            ItemBase * wire2 = addSetWire(modelset->getItem(gndT[0].first.label), gndT[0].first.connectorID, m_addedDefaultPart, gndConnectorID, transparent);
            Wire * w2 = qobject_cast<Wire *>(wire2);
            w2->setColor(QColor(0, 0, 0), 1);
            m_breadBoardPin.insert("GND", gndConnectorID);
        }
    } else {

        foreach (TerminalStringPair pair, vccT) {
            if (m_breadBoardPin.contains(pair.second)) {
                QString vccConnectorID = m_breadBoardPin[pair.second];
                QList<QString> vccConnectorIDList;
                vccConnectorIDList.append(vccConnectorID);
                vccConnectorID = findBreadBoardUnused(vccConnectorIDList, vccConnectorIDList, false);
                ItemBase * wire = addSetWire(modelset->getItem(pair.first.label), pair.first.connectorID, m_addedDefaultPart, vccConnectorID, transparent);
            }
        }

        foreach (TerminalStringPair pair, gndT) {
            if (m_breadBoardPin.contains(pair.second)) {
                QString gndConnectorID = m_breadBoardPin[pair.second];
                QList<QString> gndConnectorIDList;
                gndConnectorIDList.append(gndConnectorID);
                gndConnectorID = findBreadBoardUnused(gndConnectorIDList, gndConnectorIDList, false);
                ItemBase * wire = addSetWire(modelset->getItem(pair.first.label), pair.first.connectorID, m_addedDefaultPart, gndConnectorID, transparent);
            }
        }

    }

}
*/
void SketchWidget::checkMousePressSuggestion(QGraphicsItem * item) {

    if (!m_prevModelSet.isNull()) findKeyItem(m_prevModelSet);
    bool isModelSet = false;
    ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
    if (itemBase) {
        QSharedPointer<ModelSet> modelset = itemBase->modelSet();
        if (!modelset.isNull()) {
            m_pressModelSet = modelset;
            isModelSet = true;
        }
    }

    if (!isModelSet) {
        removePrevModelSet();
        //AutoCompleter::clearRecommend();
    }
}

void SketchWidget::checkSelectSuggestion() {
    if (!m_autoComplete || m_pressModelSet.isNull()) return;
    if (m_pressModelSet != m_prevModelSet) removePrevModelSet();
    QSharedPointer<SetConnection> setConnection = m_pressModelSet->setConnection();
    if (!m_pressModelSet->isConfirm()) {
        //confirm!
        if (setConnection.isNull()) {
            //complete model set only
            selectModelSet(m_pressModelSet, false);
        } else {
            //complete module to module
            selectSetToSet(m_pressModelSet, setConnection, true, false);
        }
    } else {
        if (!setConnection.isNull()) {
            if (setConnection->isConfirm()) {
                // suggest next
                AutoCompleter::getSuggestionNext(m_pressModelSet, this);
            } else {
                // A-B wiring confirm
                newAddSetConnectionCommand(setConnection, NULL);
            }
        } else {
            AutoCompleter::getSuggestionNext(m_pressModelSet, this);
        }
    }
//    //TODO: SET to SET
//    if (m_pressModelSet->setConnection().isNull()) {
//        selectModelSet(m_pressModelSet, false);
//    } else {
//        setOpacity(m_pressModelSet);
//        confirmSelect(m_pressModelSet);
//    }

//    //setOpacity(m_pressModelSet);
//    //confirmSelect(m_pressModelSet);
}

void SketchWidget::confirmSelect(QSharedPointer<ModelSet> modelSet) {

//    modelSet->setConfirm();
//    if (!modelSet->setConnection().isNull())
//        modelSet->setConnection()->setConfirm();
//    if (!modelSet->breadboardConnection().isNull()) {
//        modelSet->breadboardConnection()->setConfirm();
//    }
    m_prevModelSet.reset();
    m_pressModelSet.reset();
    /*
    if (modelSet->keyId() != -1) {
        ItemBase * keyItem = findItem(modelSet->keyId());
        if (keyItem != NULL) modelSet->setKeyItem(keyItem);
    }*/
    //check for exist 
    //AutoCompleter::getSuggestionExist(modelSet, this);
    AutoCompleter::getSuggestionNext(modelSet, this);
    m_savedModelSet.append(modelSet);
    return;
}

void SketchWidget::setOpacity(QSharedPointer<ModelSet> modelSet) {
    if (modelSet.isNull()) return;
    if (!modelSet->isConfirm()) {
        modelSet->setConfirm();

        QList<ItemBase *> itemList = modelSet->getItemList();
        //if (itemList.length() == 0) return;
        foreach(ItemBase * item, itemList) {
            if (item->opacity() == 1) continue;
            item->setOpacity(1);
            QPair<ModelSet::Terminal, ModelSet::Terminal> tpair = modelSet->getWireConnection(item);
            ModelSet::Terminal from = tpair.first;
            ModelSet::Terminal to = tpair.second;
            if (from.label != "" && to.label != "") {
                ItemBase * fromItem = modelSet->getItem(from.label);
                ItemBase * toItem = modelSet->getItem(to.label);
                changeConnection(fromItem->id(), from.connectorID, item->id(), "connector0", ViewLayer::specFromID(fromItem->findConnectorItemWithSharedID(from.connectorID)->attachedToViewLayerID()), true, false, false);
                changeConnection(toItem->id(), to.connectorID, item->id(), "connector1",ViewLayer::specFromID(toItem->findConnectorItemWithSharedID(to.connectorID)->attachedToViewLayerID()), true, false, false);

            }
        }
    }
    //setOpacity(modelSet->setConnection());
    //setOpacity(modelSet->breadboardConnection());
}

void SketchWidget::setOpacity(QSharedPointer<SetConnection> setConnection) {
    if (setConnection.isNull()) return;
    if (setConnection->isConfirm()) return;
    
    setConnection->setConfirm();
    QSharedPointer<ModelSet> fromModelSet = setConnection->getFromModelSet();
    QSharedPointer<ModelSet> toModelSet = setConnection->getToModelSet();
    fromModelSet->appendSetConnectionList(0, setConnection);
    toModelSet->appendSetConnectionList(1, setConnection);

    QList<ItemBase *> itemList = setConnection->getWireList();
    //if (itemList.length() == 0) return;
    foreach(ItemBase * item, itemList) {
        if (item->opacity() == 1) continue;
        item->setOpacity(1);
        QPair<ModelSet::Terminal, ModelSet::Terminal> tpair = setConnection->getWireConnection(item);
        ModelSet::Terminal from = tpair.first;
        ModelSet::Terminal to = tpair.second;
        if (from.label != "" && to.label != "") {
            ItemBase * fromItem = fromModelSet->getItem(from.label);
            ItemBase * toItem = toModelSet->getItem(to.label);
            changeConnection(fromItem->id(), from.connectorID, item->id(), "connector0", ViewLayer::specFromID(fromItem->findConnectorItemWithSharedID(from.connectorID)->attachedToViewLayerID()), true, false, false);
            changeConnection(toItem->id(), to.connectorID, item->id(), "connector1",ViewLayer::specFromID(toItem->findConnectorItemWithSharedID(to.connectorID)->attachedToViewLayerID()), true, false, false);

        }
    }
    
}

QList<QSharedPointer<ModelSet>> SketchWidget::getSavedModelSets() {
    return m_savedModelSet;
}
