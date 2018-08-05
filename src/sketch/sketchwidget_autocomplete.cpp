
#include <QtCore>
#include <QGraphicsScene>
#include <QGraphicsView>


#include "sketchwidget.h"
#include "../autocomplete/autocompleter.h"
#include "../autocomplete/suggestion.h"
#include "../debugdialog.h"
#include "../connectors/connectoritem.h"
#include "../waitpushundostack.h"


Suggestion * SketchWidget::getSuggestionFromHash(ItemBase * itemBase) {
    if (!m_suggestionHash.contains(itemBase)) return NULL;
    return m_suggestionHash[itemBase];
}

void SketchWidget::setAutoComplete(bool autoComplete) {
	m_autoComplete = autoComplete;
}

void SketchWidget::getSuggestions() {
    getSuggestions(m_droppingItem);
}

void SketchWidget::getSuggestions(ItemBase *itemBase) {
	if (!m_autoComplete) return;

	//QList<Suggestion *> suggestionList = 
	AutoCompleter::getSuggestions(itemBase->id(), m_suggestionList, this); 
	//one at a time or all?
    foreach(Suggestion * suggestion, m_suggestionList) {
        SuggestionThing * suggestionThing = suggestion->getSTNext();


		if (suggestionThing != NULL) {

            long fromID = suggestion->getFromID();
            QString fromConnectorID = suggestion->getFromConnectorID();
            QList<SuggestionThing::ToModule *> toModuleList = suggestionThing->getToModuleList();
            DebugDialog::debug(QString("fromID:%1, fromConnectorID:%2, toModuleListLen:%3")
            	.arg(fromID).arg(fromConnectorID).arg(toModuleList.count()));
            foreach(SuggestionThing::ToModule * toModule, toModuleList) {
                DebugDialog::debug(QString("AddSuggestion:%1, %2 (%3)").arg(toModule->moduleID).arg(toModule->connectorID).arg(suggestionThing->getTypeString()));
                ItemBase * itemBase = AddSuggestion(fromID, fromConnectorID, toModule->moduleID, toModule->connectorID);
                if (itemBase != NULL) {
                	m_suggestionHash[itemBase] = suggestion;
                	toModule->itemBase = itemBase;
                }
            }
		}
	}

}

ItemBase * SketchWidget::AddSuggestion(long fromID, const QString & fromConnectorID, QString toModuleID, const QString & toConnectorID) { // TODO: add offset, paras
	
	
	//toModuleID = QString("ResistorModuleID");

    //QString toConnectorID2 = QString("connector1");
	// get from item
	ItemBase * fromItem = findItem(fromID);
	if (fromItem == NULL) return NULL;

    ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, fromItem->viewLayerPlacement());
	if (fromConnectorItem == NULL) return NULL; // for now

	//check connection item under
	QList<ConnectorItem *> exclude;
    ConnectorItem * fromOverConnectorItem = fromConnectorItem->findConnectorUnder(true, true, exclude, true, fromConnectorItem);

    //TODO fromOverConnectorItem == NULL?
	if (fromOverConnectorItem != NULL) {
        //DebugDialog::debug(QString("fromOverConnectorItem:%1").arg(fromOverConnectorItem->connectorSharedID()));
		fromOverConnectorItem->setOverConnectorItem(NULL);   // clean up
		toModuleID = checkDroppedModuleID(toModuleID);
		ModelPart * modelPart = m_referenceModel->retrieveModelPart(toModuleID);
		if (modelPart ==  NULL) return NULL;
		if (!canDropModelPart(modelPart)) return NULL;


		ViewGeometry viewGeometry;
		QPointF fromOverPos = fromOverConnectorItem->sceneAdjustedTerminalPoint(NULL);

		long toID = ItemBase::getNextID();
		bool doConnectors = true;
		ItemBase *toItem = addItemAuxTemp(modelPart, defaultViewLayerPlacement(modelPart), viewGeometry, toID, doConnectors, m_viewID, true);
        
        ConnectorItem * toConnectorItem = findConnectorItem(toItem, toConnectorID, toItem->viewLayerPlacement());
		QPointF toPos = toItem->getViewGeometry().loc();
		QPointF toConnectorPos = toConnectorItem->sceneAdjustedTerminalPoint(NULL);
		ModelPart * wirePart = m_referenceModel->retrieveModelPart(QString("WireModuleID"));
		long wireID = ItemBase::getNextID();
		ViewGeometry viewGeometryWire;
		QLineF line;
		QPointF startPos;

		if (modelPart->family() != "microcontroller board (arduino)") {
		//if (toModuleID != "arduino_Uno_Rev3(fix)") {
			QPointF offset(0, 0);
			startPos = fromOverPos + offset;
            QPointF terminalOffset = toPos-toConnectorPos;
            QPointF fromCenter = fromItem->getViewGeometry().loc()+fromItem->boundingRect().center();
            if (fromCenter.x()- fromOverPos.x() > 0) {
				offset.setX(-36);
			} else {
				offset.setX(36);
			}

			toItem->setPos(fromOverPos+terminalOffset+offset);

			line.setLine(0, 0, offset.x(), 0);
		} else {
			startPos = fromOverPos;
		    //sceneAdjustedTerminalPoint
		    line.setLine(0, 0, -startPos.x()-toPos.x()+toConnectorPos.x(), -startPos.y()-toPos.y()+toConnectorPos.y());
		    
		}
		viewGeometryWire.setLoc(startPos);
		viewGeometryWire.setLine(line);
	    ItemBase * wire = addItemAuxTemp(wirePart, defaultViewLayerPlacement(wirePart), viewGeometryWire, wireID, doConnectors, m_viewID, true);
	   
	    wire->setOpacity(0.5);
		toItem->setOpacity(0.5);

        changeConnection(fromOverConnectorItem->attachedTo()->id(), fromOverConnectorItem->connectorSharedID(), wireID, "connector0", ViewLayer::specFromID(fromOverConnectorItem->attachedToViewLayerID()), true, false, false);
		changeConnection(toID, toConnectorID, wireID, "connector1",ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()), true, false, false);
		//extendChangeConnectionCommand(BaseCommand::CrossView, toConnectorItem, wire->connector0, ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()), true, parentCommand);
        return toItem;
	}
    return NULL;

}

void SketchWidget::mousePressSuggestionEvent(QGraphicsItem * item) {

    if (!m_autoComplete) return;
    if (item->opacity() == 1) return;
    ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
    itemBase->setOpacity(1);
    if (itemBase->family() == "microcontroller board (arduino)") return;
    //temp: delete ALL
    /*
    foreach(QGraphicsItem * item, scene()->items()) {
        if (item->opacity() < 1) delete item;
    }*/
    //delete *itemBase in toModule


    getSuggestions(itemBase);
    //delete QHash?

}


void SketchWidget::mousePressSuggestionEvent(QMouseEvent *event, QGraphicsItem * item) {

    if (!m_autoComplete) return;
    if (item->opacity() == 1) return;
    ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
    if (itemBase) {

	    ModelPart * modelPart = itemBase->modelPart();
		if (modelPart == NULL) return;
		if (modelPart->modelPartShared() == NULL) return;

		ModelPart::ItemType itemType = modelPart->itemType();

        QUndoCommand* parentCommand = new TemporaryCommand(tr("Add %1").arg(itemBase->title()));

		stackSelectionState(false, parentCommand);
		CleanUpWiresCommand * cuw = new CleanUpWiresCommand(this, CleanUpWiresCommand::Noop, parentCommand);
		new CleanUpRatsnestsCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

		itemBase->saveGeometry();
		ViewGeometry viewGeometry = itemBase->getViewGeometry();

		long fromID = itemBase->id();
        BaseCommand::CrossViewType crossViewType = BaseCommand::CrossView;
        ViewLayer::ViewLayerPlacement viewLayerPlacement = itemBase->viewLayerPlacement();
        AddItemCommand * addItemCommand = newAddItemCommand(crossViewType, modelPart, modelPart->moduleID(), viewLayerPlacement, viewGeometry, fromID, true, -1, true, parentCommand);
		addItemCommand->setDropOrigin(this);
		new CheckStickyCommand(this, crossViewType, fromID, false, CheckStickyCommand::RemoveOnly, parentCommand);

		SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
		selectItemCommand->addRedo(fromID);

		new ShowLabelFirstTimeCommand(this, crossViewType, fromID, true, true, parentCommand);

		/*
		if (modelPart->itemType() == ModelPart::Wire && !m_lastColorSelected.isEmpty()) {
			new WireColorChangeCommand(this, fromID, m_lastColorSelected, m_lastColorSelected, 1.0, 1.0, parentCommand);
		}*/

		bool gotConnector = false;
		ConnectorItem * firstConnectorTo = NULL;
		
		foreach (ConnectorItem * connectorItem, itemBase->cachedConnectorItems()) {
			//connectorItem->setMarked(false);
			//connectorItems.append(connectorItem);
			ConnectorItem * to = connectorItem->overConnectorItem();

			if (firstConnectorTo == NULL) firstConnectorTo = to;
			if (to != NULL) {
				to->connectorHover(to->attachedTo(), false);
				connectorItem->setOverConnectorItem(NULL);   // clean up
				extendChangeConnectionCommand(BaseCommand::CrossView, connectorItem, to, ViewLayer::specFromID(connectorItem->attachedToViewLayerID()), true, parentCommand);
				gotConnector = true;
			}
			//connectorItem->clearConnectorHover();
        }
        itemBase->removeLayerKin();
        this->scene()->removeItem(itemBase);
        
        delete itemBase->modelPart();
		delete itemBase;

		if (gotConnector) {
		    new CleanUpRatsnestsCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
			new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
			cuw->setDirection(CleanUpWiresCommand::UndoOnly);
		}

		if (itemType == ModelPart::CopperFill) {
		    m_undoStack->waitPushTemporary(parentCommand, 10);
		}
		else {
		    m_undoStack->waitPush(parentCommand, 10);
		}

        ItemBase * newItem = findItem(fromID);
        if (newItem != NULL) {
        	DebugDialog::debug("findItem!!!!!!!!!!!!!!!!");
        	getSuggestions(newItem);
        }
	
	}

}


void SketchWidget::testAdd(QString moduleID, ConnectorItem * to) {

    //test2();
    //return;
	if (to == NULL) return;

    //QUndoCommand * parentCommand = new QUndoCommand(tr("testAdd"));
    //new CleanUpWiresCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);
    //new CleanUpRatsnestsCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);


	QPointF offset(0, 0);
    moduleID = checkDroppedModuleID(moduleID);
	ModelPart * modelPart = m_referenceModel->retrieveModelPart(moduleID);
    if (modelPart ==  NULL) return;

    if (!canDropModelPart(modelPart)) return;

	//m_droppingWire = (modelPart->itemType() == ModelPart::Wire);
	m_droppingOffset = offset;


	ViewGeometry viewGeometry;

    //QPointF p = to->rect().center() - offset;
    QPointF p = to->sceneAdjustedTerminalPoint(NULL);
    //QPointF currentParentPos = itemBase->mapToParent(itemBase->mapFromScene(scenePos));
	//sQPointF buttonDownParentPos = itemBase->mapToParent(itemBase->mapFromScene(m_mousePressScenePos));
	viewGeometry.setLoc(p);
	/*
    foreach(ConnectorItem * connectorItem, to->attachedTo()->cachedConnectorItems()) {
		DebugDialog::debug(QString("connectorItem:%1").arg(connectorItem->connectorSharedID()));
	}*/


	long fromID = ItemBase::getNextID();

	bool doConnectors = true;
	ItemBase *testItem;

	testItem = addItemAuxTemp(modelPart, defaultViewLayerPlacement(modelPart), viewGeometry, fromID, doConnectors, m_viewID, true);
    QSizeF size = testItem->sceneBoundingRect().size();
    testItem->setOpacity(0.5);
    foreach(ConnectorItem * connectorItem, testItem->cachedConnectorItems()) {
        DebugDialog::debug(QString("--connectorItem:%1").arg(connectorItem->connectorSharedName()));
    }

    ModelPart * wirePart = m_referenceModel->retrieveModelPart(QString("WireModuleID"));
    long wireID = ItemBase::getNextID();
    //sceneAdjustedTerminalPoint
    ViewGeometry viewGeometryWire;
    QLineF line(0, 0, -5, 0);
    viewGeometryWire.setLoc(p);
	viewGeometryWire.setLine(line);
    ItemBase * wire = addItemAuxTemp(wirePart, defaultViewLayerPlacement(wirePart), viewGeometryWire, wireID, doConnectors, m_viewID, true);
	/*
	new AddItemCommand(this, BaseCommand::CrossView, ModuleIDNames::WireModuleIDName, testItem->viewLayerPlacement(), viewGeometryWire, wireID, false, -1, parentCommand);
	new ChangeConnectionCommand(this, BaseCommand::CrossView,
								testItem->id(), to->connectorSharedID(),
								wireID, "connector0",
                                testItem->viewLayerPlacement(), true, parentCommand);*/
   	//the other side
    //wire->setLine(p.x(), p.y(), )
    wire->setOpacity(0.5);

    //TODO:HERE!!!
    /*
    QList<ConnectorItem *> connectorItems;
    connectorItems.append(a);
    ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::RatsnestFlag);
	foreach (ConnectorItem * connectorItem, connectorItems) {
		DebugDialog::debug(QString("from collectEqualPotential: %1").arg(connectorItem->connectorSharedID()));
	}*/


	//init line


    //testItem->setPos(testItem->getViewGeometry().loc() + QPointF(size.width() / 2, size.height() / 2));

	//QHash<long, ItemBase *> savedItems;
	//QHash<Wire *, ConnectorItem *> savedWires;
	//findAlignmentAnchor(testItem, savedItems, savedWires);

	
	/*

	
	
	ModelPart::ItemType itemType = modelPart->itemType();

	QUndoCommand* parentCommand = new TemporaryCommand(tr("Add %1").arg(m_droppingItem->title()));

	stackSelectionState(false, parentCommand);
	CleanUpWiresCommand * cuw = new CleanUpWiresCommand(this, CleanUpWiresCommand::Noop, parentCommand);
	new CleanUpRatsnestsCommand(this, CleanUpWiresCommand::UndoOnly, parentCommand);

	//m_droppingItem->saveGeometry();
	//ViewGeometry viewGeometry = m_droppingItem->getViewGeometry();
	ViewGeometry viewGeometry;
    QPointF p = QPointF(0,0) - offset;
	viewGeometry.setLoc(p);


	//long fromID = m_droppingItem->id();
	long fromID = ItemBase::getNextID();

	BaseCommand::CrossViewType crossViewType = BaseCommand::CrossView;
	switch (modelPart->itemType()) {
		case ModelPart::Ruler:
		case ModelPart::Note:
			// rulers and notes are local to a particular view
			crossViewType = BaseCommand::SingleView;
			break;
		default:
			break;				
	}

    ViewLayer::ViewLayerPlacement viewLayerPlacement;
    getDroppedItemViewLayerPlacement(modelPart, viewLayerPlacement);  
	AddItemCommand * addItemCommand = newAddItemCommand(crossViewType, modelPart, modelPart->moduleID(), viewLayerPlacement, viewGeometry, fromID, true, -1, true, parentCommand);
	addItemCommand->setDropOrigin(this);

	new SetDropOffsetCommand(this, fromID, m_droppingOffset, parentCommand);
	
	new CheckStickyCommand(this, crossViewType, fromID, false, CheckStickyCommand::RemoveOnly, parentCommand);

	SelectItemCommand * selectItemCommand = new SelectItemCommand(this, SelectItemCommand::NormalSelect, parentCommand);
	selectItemCommand->addRedo(fromID);

	new ShowLabelFirstTimeCommand(this, crossViewType, fromID, true, true, parentCommand);

	if (modelPart->itemType() == ModelPart::Wire && !m_lastColorSelected.isEmpty()) {
		new WireColorChangeCommand(this, fromID, m_lastColorSelected, m_lastColorSelected, 1.0, 1.0, parentCommand);
	}

	bool gotConnector = false;

	// jrc: 24 aug 2010: don't see why restoring color on dropped item is necessary
	//QList<ConnectorItem *> connectorItems;
	foreach (ConnectorItem * connectorItem, m_droppingItem->cachedConnectorItems()) {
		//connectorItem->setMarked(false);
		//connectorItems.append(connectorItem);
		ConnectorItem * to = connectorItem->overConnectorItem();
		if (to != NULL) {
			to->connectorHover(to->attachedTo(), false);
			connectorItem->setOverConnectorItem(NULL);   // clean up
			extendChangeConnectionCommand(BaseCommand::CrossView, connectorItem, to, ViewLayer::specFromID(connectorItem->attachedToViewLayerID()), true, parentCommand);
			gotConnector = true;
		}
		//connectorItem->clearConnectorHover();
	}
	//foreach (ConnectorItem * connectorItem, connectorItems) {
		//if (!connectorItem->marked()) {
			//connectorItem->restoreColor(false, 0, true);
		//}
	//}
	//m_droppingItem->clearConnectorHover();

	//clearTemporaries();

	//killDroppingItem();

	if (gotConnector) {
	    new CleanUpRatsnestsCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
		new CleanUpWiresCommand(this, CleanUpWiresCommand::RedoOnly, parentCommand);
		cuw->setDirection(CleanUpWiresCommand::UndoOnly);
	}

    if (itemType == ModelPart::CopperFill) {
        m_undoStack->waitPushTemporary(parentCommand, 10);
    }
    else {
        m_undoStack->waitPush(parentCommand, 10);
    }
    DebugDialog::debug(QString("testAdd m_droppingItem:%1").arg(m_droppingItem->title()));
	*/
	
}
