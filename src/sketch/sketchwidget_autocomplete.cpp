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
    m_breadBoardModelSet = QSharedPointer<ModelSet>(new ModelSet(-1, m_addedDefaultPart->title()));
    m_breadBoardModelSet->setKeyItem(m_addedDefaultPart);
    foreach(ConnectorItem * connectorItem, m_addedDefaultPart->cachedConnectorItems()) {
        DebugDialog::debug(QString("---%1").arg(connectorItem->connectorSharedID()));
        ModelSet::Terminal t1 = ModelSet::Terminal(m_addedDefaultPart->moduleID(), m_addedDefaultPart->title(), "", connectorItem->connectorSharedID(), "");
        ModelSet::Terminal t2 = ModelSet::Terminal("NULL", "NULL", "NULL", "NULL", "");
        m_breadBoardModelSet->appendConnection(t1, t2);
        m_breadBoardModelSet->insertTerminalnameHash(connectorItem->connectorSharedID(), t1);
    }
}

void SketchWidget::addSetToSet(QSharedPointer<ModelSet> modelSet, QSharedPointer<SetConnection> setconnection, bool transparent) {
    QSharedPointer<ModelSet> temp = m_prevModelSet;
    addModelSet(modelSet, transparent);
    if (transparent || temp!=modelSet) addSetConnection(setconnection, transparent);
    modelSet->addSetConnection(setconnection);
}

void SketchWidget::addSetConnection(QSharedPointer<SetConnection> setconnection, bool transparent) {
    QSharedPointer<ModelSet> from = setconnection->getFromModelSet();
    QSharedPointer<ModelSet> to = setconnection->getToModelSet();
    if (from.isNull() || to.isNull()) return;
    findKeyItem(from);
    QList<QString> usedConnectorID;

	QList<SetConnection::Connection> connectionList = setconnection->getConnectionList();
    foreach(SetConnection::Connection c, connectionList) {
        QPair<ItemBase *, QString> p1 = from->getItemAndCID(c.fromTerminal);
        QPair<ItemBase *, QString> p2;
        QList<QPair<ModelSet::Terminal, QString>> pintypeT = to->getPinTypeTerminal(c.toTerminal);
        QList<QPair<ModelSet::Terminal, QString>> breadboardT = m_breadBoardModelSet->getPinTypeTerminal(c.toTerminal);
        if (to->isMicrocontroller() && breadboardT.length() > 0) {
            ModelSet::Terminal t = breadboardT[0].first;
            QString vccConnectorID = t.connectorID;
            QList<QString> vccConnectorIDList;
            vccConnectorIDList.append(vccConnectorID);
            vccConnectorID = findBreadBoardUnused(vccConnectorIDList, vccConnectorIDList, false);
            p2 = QPair<ItemBase *, QString>(m_addedDefaultPart, vccConnectorID);
        } else if (to->isMicrocontroller() && pintypeT.length() > 0) {

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
        ItemBase * wire = addSetWire(p1.first, p1.second, p2.first, p2.second, transparent);
        if (wire == NULL) continue;
        if (c.changeColor) {
            Wire * w = qobject_cast<Wire *>(wire);
            w->setColor(c.color, 1);
        }
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
            removePrevSetConnection(false);
        }
        return;
    }
	removePrevModelSet();

    if (modelSet.isNull()) return;
    m_prevModelSet = modelSet;
    findKeyItem(modelSet);

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
        //Wire * w = qobject_cast<Wire *>(wire);
        modelSet->addItem(wire);
        //TODO: arduino
    }
    completeSuggestion(modelSet, transparent);
    if (!transparent) confirmSelect(modelSet);

}

void SketchWidget::findKeyItem(QSharedPointer<ModelSet> modelSet) {
    //TODO: change to command type
    ItemBase * keyItem = modelSet->keyItem();
    long id = modelSet->keyId();
    keyItem = findItem(id);
    if (keyItem != NULL) {
        modelSet->setKeyItem(keyItem);
        keyItem->setModelSet(modelSet);
    }

}

void SketchWidget::removePrevModelSet() {

    if (m_prevModelSet.isNull()) return;
    if (!m_prevModelSet->isConfirm()) {
    QList<ItemBase *> itemList = m_prevModelSet->getItemList();
        foreach(ItemBase * itemBase, itemList) {
            //itemBase->removeLayerKin();
            this->scene()->removeItem(itemBase);
            if (itemBase->modelPart()) {
                delete itemBase->modelPart();
            }
            delete itemBase;
        }
    }
    removePrevSetConnection(true);
	m_prevModelSet->emptyItemList();
    m_prevModelSet.clear();

}

void SketchWidget::removePrevSetConnection(bool removeBreadboard) {
    if (m_prevModelSet.isNull()) return;
    QSharedPointer<SetConnection> setConnection = m_prevModelSet->setConnection();
    QSharedPointer<SetConnection> breadboardConnection = m_prevModelSet->breadboardConnection();
    if (setConnection.isNull() && breadboardConnection.isNull()) return;
    QList<ItemBase *> itemList;
    if (!setConnection.isNull()) itemList.append(setConnection->getWireList());
    if (!breadboardConnection.isNull() && removeBreadboard) itemList.append(breadboardConnection->getWireList());

    foreach(ItemBase * itemBase, itemList) {
        //itemBase->removeLayerKin();
        this->scene()->removeItem(itemBase);
        if (itemBase->modelPart()) {
            delete itemBase->modelPart();
        }
        delete itemBase;
    }
    if (!setConnection.isNull()) setConnection->emptyWireList();
    if (!breadboardConnection.isNull() && removeBreadboard) breadboardConnection->emptyWireList();
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
            setconnection = QSharedPointer<SetConnection>(new SetConnection(modelset, m_breadBoardModelSet));
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
                setconnection->appendConnection(vccPair.second, vccConnectorID, QColor(255, 0, 0));
                m_breadBoardModelSet->insertTerminalType(vccConnectorID, "VCC");
                m_breadBoardModelSet->insertTerminalType(vccConnectorID, vccPair.second);
            }
            if (gndT.length() > 0 && gndConnectorID!="") {
                setconnection->appendConnection(gndT[0].second, gndConnectorID, QColor(0, 0, 0));
                //setconnection->appendConnection(gndT[0].first.connectorID, gndConnectorID, QColor(0, 0, 0));
                m_breadBoardModelSet->insertTerminalType(gndConnectorID, gndT[0].second);
            }
            modelset->addBreadboardConnection(setconnection);
        }

    }
    if (!setconnection.isNull()) {
        addSetConnection(setconnection, transparent);
        modelset->addSetConnection(setconnection);
    }

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
    if (m_pressModelSet != m_prevModelSet) removePrevModelSet();
    setOpacity(m_pressModelSet);
    confirmSelect(m_pressModelSet);
}

void SketchWidget::confirmSelect(QSharedPointer<ModelSet> modelSet) {

    
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
        if (itemList.length() == 0) return;
        foreach(ItemBase * item, itemList) {
            if (item->opacity() == 1) continue;
            item->setOpacity(1);
        }
    }
    setOpacity(modelSet->setConnection());
    setOpacity(modelSet->breadboardConnection());
}

void SketchWidget::setOpacity(QSharedPointer<SetConnection> setConnection) {
    if (setConnection.isNull()) return;
    if (setConnection->isConfirm()) return;
    
    setConnection->setConfirm();
    setConnection->getFromModelSet()->appendSetConnectionList(0, setConnection);
    setConnection->getToModelSet()->appendSetConnectionList(1, setConnection);

    QList<ItemBase *> itemList = setConnection->getWireList();
    if (itemList.length() == 0) return;
    foreach(ItemBase * item, itemList) {
        if (item->opacity() == 1) continue;
        item->setOpacity(1);
    }
    
}

QList<QSharedPointer<ModelSet>> SketchWidget::getSavedModelSets() {
    return m_savedModelSet;
}
