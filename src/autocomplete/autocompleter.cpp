#include "autocompleter.h"

#include "../debugdialog.h"
#include "../sketch/sketchwidget.h"
#include "../referencemodel/referencemodel.h"
#include "../model/modelpart.h"
#include "suggestion.h"
#include "../connectors/connectoritem.h"
#include "../items/partfactory.h"
#include "../items/paletteitem.h"
#include "../items/logoitem.h"
#include "../items/pad.h"
#include "../items/ruler.h"
#include "../items/symbolpaletteitem.h"
#include "../items/wire.h"
#include "../items/moduleidnames.h"
#include "../items/resistor.h"

#include <QString>
#include <QtDebug>
#include <QVariant>
#include <QHash>


AutoCompleter* AutoCompleter::singleton = NULL;

AutoCompleter::AutoCompleter() {

}

AutoCompleter::~AutoCompleter() {

}

void AutoCompleter::test() {
    //DebugDialog::
    //qDebug() << "__prob__:" << singleton->m_terminalMap["__prob__"].toString();
}

void AutoCompleter::setDatasheetMap(QVariantMap & qVariantMap) {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    singleton->setDatasheetMapSelf(qVariantMap);
}

void AutoCompleter::setHistoryMap(QVariantMap & qVariantMap) {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    singleton->setHistoryMapSelf(qVariantMap);
}

void AutoCompleter::setDatasheetMapSelf(QVariantMap & qVariantMap) {
    m_datasheetMap = qVariantMap;
}

void AutoCompleter::setHistoryMapSelf(QVariantMap & qVariantMap) {
    m_historyMap = qVariantMap;
	sortHistoryMap();    

}

void AutoCompleter::setTerminalMap(QVariantMap & qVariantMap) {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    singleton->setTerminalMapSelf(qVariantMap);
}

void AutoCompleter::setTerminalMapSelf(QVariantMap & qVariantMap) {
	//TODO: modify~~~~
	//m_terminalMap = qVariantMap;
    QVariantMap temp = qVariantMap["pair"].toMap();
    sortTerminalPairMap(temp);
    //m_terminalPairMap = ;
	m_terminalTriMap = qVariantMap["tri"].toMap();
	sortTerminalTriMap(m_terminalTriMap);

}

void AutoCompleter::setMaxSuggestion(int max) {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    singleton->setMaxSuggestionSelf(max);
}

void AutoCompleter::setMaxSuggestionSelf(int max) {
    m_maxSuggestion = max;
}

bool AutoCompleter::pairSecondComparer(const QPairSD_t &p1, const QPairSD_t &p2) {
    return p1.second > p2.second;
}

void AutoCompleter::sortHistoryMap() {
	foreach (QVariant connectFrom, m_historyMap.keys()) {
        QVariantMap connectFromMap = m_historyMap[connectFrom.toString()].toMap();
        QList<QPairSD_t> probPairList;
        foreach(QVariant connectTo, connectFromMap.keys()) {
        	QPairSD_t probPair(connectTo.toString(), connectFromMap[connectTo.toString()].toDouble());
        	probPairList.append(probPair);
            //DebugDialog::debug(QString("%1 -> %2 : %3").arg(connectFrom.toString()).arg(probPair.second));
        }

        qSort(probPairList.begin(), probPairList.end(), pairSecondComparer);
        m_historyMap[connectFrom.toString()].setValue(probPairList);
	}
}

void AutoCompleter::sortTerminalPairMap(QVariantMap & toSortMap) {

    foreach (QVariant component, toSortMap.keys()) {
        //m_terminalPairMap[component.toString()] = QVariant(QVariantMap());
        QVariantMap componentMap = toSortMap[component.toString()].toMap();
        foreach(QVariant terminal, componentMap.keys()){
            QVariantMap terminalMap = componentMap[terminal.toString()].toMap();
            QList<QPairSD_t> probPairList;
			foreach(QVariant probKey, terminalMap.keys()) {
                QPairSD_t probPair(probKey.toString(), terminalMap[probKey.toString()].toDouble());
				probPairList.append(probPair);
				//save
				//getModuleIDByTitleSelf(probKey.toString().split("_")[0]);
			}

            qSort(probPairList.begin(), probPairList.end(), pairSecondComparer);
            componentMap[terminal.toString()].setValue(probPairList);

		}
        toSortMap[component.toString()].setValue(componentMap);
	}

	m_terminalPairMap = toSortMap;

}

void AutoCompleter::sortTerminalTriMap(QVariantMap & toSortMap) {
	foreach (QVariant component, toSortMap.keys()) {
        //m_terminalPairMap[component.toString()] = QVariant(QVariantMap());
        QVariantMap componentMap = toSortMap[component.toString()].toMap();
        foreach(QVariant terminal, componentMap.keys()){
            QVariantMap terminalMap = componentMap[terminal.toString()].toMap();

            foreach(QVariant connectTo, terminalMap.keys()) {
                QVariantMap connectToMap = terminalMap[connectTo.toString()].toMap();
            	QList<QPairSD_t> probPairList;
				foreach(QVariant probKey, connectToMap.keys()) {
	                QPairSD_t probPair(probKey.toString(), connectToMap[probKey.toString()].toDouble());
					probPairList.append(probPair);
					//save
					//getModuleIDByTitleSelf(probKey.toString().split("_")[0]);
				}

	            qSort(probPairList.begin(), probPairList.end(), pairSecondComparer);
	            terminalMap[connectTo.toString()].setValue(probPairList);
            }
            componentMap[terminal.toString()].setValue(terminalMap);
		}
        toSortMap[component.toString()].setValue(componentMap);
	}

	m_terminalTriMap = toSortMap;
}

QString AutoCompleter::getModuleIDByTitle(QString title) {
	if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    return singleton->getModuleIDByTitleSelf(title);
}

QString AutoCompleter::getModuleIDByTitleSelf(QString title) {
	if (m_titleToModuleID.contains(title)) return m_titleToModuleID[title];
    else return " ";

}

QString AutoCompleter::getModuleIDByTitle(QString title, SketchWidget * sketchWidget) {
    if (m_titleToModuleID.contains(title)) return m_titleToModuleID[title];
    else {
        QString toModuleID = getModuleID(title, sketchWidget);
        m_titleToModuleID.insert(title, toModuleID);
        return toModuleID;
    }
}



void AutoCompleter::getSuggestions(long fromID, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget) {
	if (singleton == NULL) {
		singleton = new AutoCompleter();
	}
	singleton->getSuggestionsSelf(fromID, suggestionList, sketchWidget);
}

QList<Suggestion *> AutoCompleter::getSuggestions(long fromID, SketchWidget * sketchWidget) {
	if (singleton == NULL) {
		singleton = new AutoCompleter();
	}
    return singleton->getSuggestionsSelf(fromID, sketchWidget);
}

void AutoCompleter::getSuggestionsSelf(long fromID, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget) {
	ItemBase * fromItem = sketchWidget->findItem(fromID);
	if (fromItem == NULL) return;

	//check if item is from suggestion
	Suggestion * suggestion = sketchWidget->getSuggestionFromHash(fromItem);
    getSuggestionsFromDatasheet(fromID, suggestion, suggestionList, sketchWidget);

	if (suggestion != NULL) {
		//DebugDialog::debug("Check History!");
	    getSuggestionsFromHistory(fromID, suggestion, suggestionList, sketchWidget);
        //TODO: delete or not?
        suggestionList.removeOne(suggestion);
        getSuggestionsParallel(suggestion, suggestionList, sketchWidget);
        delete suggestion;
        return;
	}

	foreach(ConnectorItem * connectorItem, fromItem->cachedConnectorItems()) {
		getSuggestionsSelf(fromID, connectorItem->connectorSharedID(), suggestionList, sketchWidget);
        //if (suggestion != NULL) suggestionList.append(suggestion);
	}
}

void AutoCompleter::getSuggestionsFromDatasheet(long fromID, Suggestion * fromSuggestion, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget) {
    
    ItemBase * itemBase = sketchWidget->findItem(fromID);
    QString datasheetTitle = itemBase->title();
    QString title = datasheetTitle;
    QString label = "1";
    QString connectorID = "";
    
    if (fromSuggestion != NULL) {
        SuggestionThing * fromSuggestionThing = fromSuggestion->getSTNow();
        if (fromSuggestionThing->getType() == SuggestionThing::SuggestionThingFromDataSheet) {
            datasheetTitle = fromSuggestionThing->getDatasheetTitle();
            foreach(SuggestionThing::ToModule * toModule, fromSuggestionThing->getToModuleList()) {
                if (toModule->itemBase == itemBase) {
                    label = toModule->label;
                    connectorID = toModule->connectorID;
                }
            }
        }
        
    }
    //Label.component
    QVariantMap titleMap = m_datasheetMap[datasheetTitle].toMap();
    QHash<QString, ItemBase *> labelHash;
    //Find title pin with same Label

    QString name = QString("%1;%2").arg(label).arg(title);
    DebugDialog::debug(QString("GetFromDATASHEET name:%1").arg(name));
    if (titleMap.contains(name)) {
        QVariantMap nameMap = titleMap[name].toMap();
        foreach(QVariant pin, nameMap.keys()) {
            if (pin.toString() == connectorID) continue;

            DebugDialog::debug(QString("GetFromDATASHEET pin:%1").arg(pin.toString()));
            //long fromID, QString fromTitle, QString fromConnectorID, QList<SuggestionThing *> suggestionList
            //Suggestion * suggestion = new Suggestion();
            QList<SuggestionThing *> suggestionThingList;
            QList<QVariant> connectToList = nameMap[pin.toString()].toList();
            SuggestionThing * suggestionT = new SuggestionThing();
            foreach(QVariant c, connectToList) {
                DebugDialog::debug(QString("GetFromDATASHEET connectTo:%1").arg(c.toString()));
                QStringList connectTo = c.toString().split(";")[1].split("_");
                QString toID = getModuleIDByTitle(connectTo[0], sketchWidget);
                SuggestionThing::ToModule * toModule = suggestionT->addToModule(toID, connectTo[0], connectTo[1], *(new QList<QPairSS_t>()));
                //TODO : new constructor for MODULE
                toModule->label = c.toString().split(";")[0];
                //suggestionT->appendUpdateHash(toModule, .....);
                //goIntoDatasheet(title, c.toString(), titleMap, itemBase);
            }
            suggestionT->setDatasheetTitle(datasheetTitle);
            suggestionT->setType(SuggestionThing::SuggestionThingFromDataSheet);
            suggestionT->setProbability(1000);
            suggestionThingList.append(suggestionT);
            Suggestion * suggestion = findFromSuggestionList(fromID, pin.toString(), suggestionList);
            if (suggestion == NULL) {
                suggestion = new Suggestion(fromID, title, pin.toString(), suggestionThingList);
                suggestionList.append(suggestion);
            } else {
                suggestion->appendSTList(suggestionThingList);
            }
        }
    }

    
}

/*
void AutoCompleter::goIntoDatasheet(QString title, QString connectTo, QVariantMap titleMap, ItemBase * itemBase) {

    QStringList connectToList = connectTo.split("_");
    QList<SuggestionThing::ToModule *> toModuleList;
    //1: label.component, 2:pin
    if (titleMap.contains(connectToList[0])) {
        QVariantMap nameMap = titleMap[connectToList[0]].toMap();
        foreach(QVariant pin, nameMap.keys()) {
            if (pin.toString() == connectToList[1]) continue;

            QList<SuggestionThing *> suggestionThingList;
            QList<QString> connectToList = nameMap[pin.toString()].value<QList<QString>>();
            SuggestionThing * suggestionT = new SuggestionThing();
            foreach(QString c, connectToList) {
                QStringList connectTo = c.split(".")[0].split("_");
                
                QString toID = getModuleIDByTitle(connectTo[0], sketchWidget);
                SuggestionThing::ToModule * toModule = suggestionT->addToModule(toID, connectTo[0], connectTo[1], *(new QList<QPairSS_t>()));
                toModuleList.append(toModule);
                //suggestionT->appendUpdateHash(toModule, .....);
                
                if (connectTo[0] == title) {
                    toModule->itemBase = itemBase;
                } else {
                    goIntoDatasheet();
                }

            }
            suggestionThingList.append(suggestionT);



        } 

    }


}*/

void AutoCompleter::getSuggestionsParallel(Suggestion * suggestion, QList<Suggestion*> & suggestionList, SketchWidget * sketchWidget) {
	/*
		suggestion 	------------	ST (fromST)
		(fromSuggestion)	|		
							|---	getSuggest
	
	*/
    long fromSuggestionID = suggestion->getFromID();
    QString fromSuggestionTitle = suggestion->getFromTitle();
	QString fromSuggestionConnectorID = suggestion->getFromConnectorID();
	QString fromSTTitle = suggestion->getSTNowTitle();
	QString fromSTConnectorID = suggestion->getSTNowConnectorID();

    //TODO: change method to prevent repeatly add same suggestion
    SuggestionThing * suggestionT = suggestion->getSTNow();
    if (suggestionT->getType() == SuggestionThing::SuggestionThingParallel || 
        suggestionT->getType() == SuggestionThing::SuggestionThingFromDataSheet) return;

    DebugDialog::debug(QString("title:%1").arg(fromSuggestionTitle));
    if (!m_terminalTriMap.contains(fromSuggestionTitle)) {
    	DebugDialog::debug("noTitle");
    	return;
    }

    DebugDialog::debug(QString("connector:%1").arg(fromSuggestionConnectorID));
    QVariantMap titleMap = m_terminalTriMap[fromSuggestionTitle].toMap();
    if (!titleMap.contains(fromSuggestionConnectorID)) {
    	DebugDialog::debug("noConnectorID");
	    return;
    }

    QString connectName = QString("%1_%2").arg(fromSTTitle).arg(fromSTConnectorID);
    DebugDialog::debug(QString("connectName:%1").arg(connectName));
    QVariantMap connectorMap = titleMap[fromSuggestionConnectorID].toMap();
    if (!connectorMap.contains(connectName)){
    	DebugDialog::debug("noConnectName");
    }
   	QList<QPairSD_t> probPairList = connectorMap[connectName].value<QList<QPairSD_t>>();
   	QList<SuggestionThing *> suggestionThingList;
   	pairsToSuggestionThings(probPairList, suggestionThingList, SuggestionThing::SuggestionThingParallel, sketchWidget);

    //TODO: new or use origin one;
    Suggestion * suggestionF = findFromSuggestionList(fromSuggestionID, fromSuggestionConnectorID, suggestionList);
    if (suggestionF == NULL) {
        suggestionF = new Suggestion(fromSuggestionID, fromSuggestionTitle, fromSuggestionConnectorID, suggestionThingList);
        suggestionList.append(suggestionF);
    } else {
        suggestionF->appendSTList(suggestionThingList);
    }

}

void AutoCompleter::getSuggestionsFromHistory(long fromID, Suggestion * suggestion, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget) {
    long fromSuggestionID = suggestion->getFromID();
	//ItemBase * fromSuggestionItem = sketchWidget->findItem(fromSuggestionID);
    QString fromSuggestionTitle = suggestion->getFromTitle();
	QString fromSuggestionConnectorID = suggestion->getFromConnectorID();
    SuggestionThing * suggestionT = suggestion->getSTNow();
    //TODO return?????
    /*
    if (suggestionT.getType() == SuggestionThing::SuggestionThingFromDataSheet) {
        return;
    }*/
	QString fromSTTitle = suggestion->getSTNowTitle();
	QString fromSTConnectorID = suggestion->getSTNowConnectorID();

	QString connectName = QString("%1_%2.%3_%4").arg(fromSuggestionTitle).arg(fromSuggestionConnectorID).arg(fromSTTitle).arg(fromSTConnectorID);
	QString connectName2 = QString("%1_%2.%3_%4").arg(fromSTTitle).arg(fromSTConnectorID).arg(fromSuggestionTitle).arg(fromSuggestionConnectorID);

	//TODO: check if fromSuggestionConnectorID connects sth
    //temp comment 
    historyToSuggestions(connectName, fromSuggestionID, suggestionList, sketchWidget);
    historyToSuggestions(connectName2, fromID, suggestionList, sketchWidget);
	
}

void AutoCompleter::historyToSuggestions(QString connectName, long fromID, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget) {
    if (m_historyMap.contains(connectName)) {

        QList<QPairSD_t> probPairList = m_historyMap[connectName].value<QList<QPairSD_t>>();
		foreach(QPairSD_t p, probPairList) {
			DebugDialog::debug(QString("History Suggest: %1, %2").arg(connectName).arg(p.first));
            QString connectToName = p.first;
			QStringList connectToNameList = connectToName.split(".");
			QStringList from = connectToNameList[0].split("_");
			QStringList to = connectToNameList[1].split("_");
			QString toID = getModuleIDByTitle(to[0], sketchWidget);
			
			if (toID == " ") continue;

			QList<SuggestionThing *> suggestionThingList;
			SuggestionThing * suggestionT = new SuggestionThing();
            suggestionT->setType(SuggestionThing::SuggestionThingFromHistory);
            suggestionT->addToModule(toID, to[0], to[1]);
            suggestionT->setProbability(p.second);
			suggestionThingList.append(suggestionT);
			Suggestion * suggestion = findFromSuggestionList(fromID, from[1], suggestionList);
			if (suggestion == NULL) {
                suggestion = new Suggestion(fromID, from[0], from[1], suggestionThingList);
                suggestionList.append(suggestion);
			} else {
                suggestion->appendSTList(suggestionThingList);
			}
			
            
		}
	}

}

Suggestion * AutoCompleter::findFromSuggestionList(long fromID, QString fromConnectorID, QList<Suggestion *> & suggestionList) {
	foreach(Suggestion * s, suggestionList) {
		if (s->getFromID() == fromID && s->getFromConnectorID() == fromConnectorID) {
			return s;
		}
	}
	return NULL;
}

QList<Suggestion *> AutoCompleter::getSuggestionsSelf(long fromID, SketchWidget * sketchWidget) {
	QList<Suggestion *> suggestionList;
    getSuggestionsSelf(fromID, suggestionList, sketchWidget);
	/*
	ItemBase * fromItem = sketchWidget->findItem(fromID);
    if (fromItem != NULL) {
    	foreach(ConnectorItem * connectorItem, fromItem->cachedConnectorItems()) {
    		Suggestion * suggestion = getSuggestionsSelf(fromID, connectorItem->connectorSharedID(), sketchWidget);
            if (suggestion != NULL) suggestionList.append(suggestion);
    	}
    }*/
   	return suggestionList;

}

Suggestion* AutoCompleter::getSuggestions(long fromID, QString fromConnectorID, SketchWidget * sketchWidget) {
	if (singleton == NULL) {
        singleton = new AutoCompleter();
	}
    //if (singleton->m_terminalMap == NULL) return;
    return singleton->getSuggestionsSelf(fromID, fromConnectorID, sketchWidget);

}

void AutoCompleter::getSuggestions(long fromID, QString fromConnectorID, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget) {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    //if (singleton->m_terminalMap == NULL) return;
    singleton->getSuggestionsSelf(fromID, fromConnectorID, suggestionList, sketchWidget);

}

void AutoCompleter::getSuggestionsSelf(long fromID, QString fromConnectorID, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget) {
    ItemBase * fromItem = sketchWidget->findItem(fromID);
    if (fromItem == NULL) return;

    // check for terminal map

    //for only pair
    

    //check for who shouldn't autocomplet single
    if (fromItem->family() == "resistor" || 
        fromItem->family() == "capacitor [bidirectional]" || 
        fromItem->family() == "capacitor [unidirectional]") {
        return;
    }

    QString title = fromItem->title();
    if (m_terminalPairMap.contains(title)) {
        DebugDialog::debug(QString("title:%1").arg(title));
        DebugDialog::debug(QString("connector:%1").arg(fromConnectorID));
        QVariantMap titleMap = m_terminalPairMap[title].toMap();
        if (titleMap.contains(fromConnectorID)) {

            QList<QPairSD_t> probPairList = titleMap[fromConnectorID].value<QList<QPairSD_t>>();
            QList<SuggestionThing *> suggestionThingList;
            pairsToSuggestionThings(probPairList, suggestionThingList, SuggestionThing::SuggestionThingSingle, sketchWidget);

            Suggestion * suggestion = findFromSuggestionList(fromID, fromConnectorID, suggestionList);
            if (suggestion == NULL) {
                suggestion = new Suggestion(fromID, title, fromConnectorID, suggestionThingList);
                suggestionList.append(suggestion);
            } else {
                suggestion->appendSTList(suggestionThingList);
            }
            return;

        } else {
            DebugDialog::debug("noConnectorID");
            return;
        }
    } else {
        DebugDialog::debug("noTitle");
        return;
    }
}

Suggestion* AutoCompleter::getSuggestionsSelf(long fromID, QString fromConnectorID, SketchWidget * sketchWidget) {
    ItemBase * fromItem = sketchWidget->findItem(fromID);
    if (fromItem == NULL) return NULL;

	// check for terminal map

	//for only pair
	QString title = fromItem->title();
	if (m_terminalPairMap.contains(title)) {
	    DebugDialog::debug(QString("title:%1").arg(title));
	    DebugDialog::debug(QString("connector:%1").arg(fromConnectorID));
        QVariantMap titleMap = m_terminalPairMap[title].toMap();
	    if (titleMap.contains(fromConnectorID)) {

            QList<QPairSD_t> probPairList = titleMap[fromConnectorID].value<QList<QPairSD_t>>();
            QList<SuggestionThing *> suggestionThingList;
            pairsToSuggestionThings(probPairList, suggestionThingList, SuggestionThing::SuggestionThingSingle, sketchWidget);

            //Suggestion * suggestion = findFromSuggestionList(fromID, fromConnectorID, suggestionList);
            //if (suggestion == NULL) {
            Suggestion * suggestion = new Suggestion(fromID, title, fromConnectorID, suggestionThingList);
            //} else {
            //    suggestion->appendSTList(suggestionThingList);
            //}
            return suggestion;

	    } else {
	    	DebugDialog::debug("noConnectorID");
	    	return NULL;
	    }
	} else {
		DebugDialog::debug("noTitle");
		return NULL;
	}

	//check for history map
}

void AutoCompleter::pairsToSuggestionThings(QList<QPairSD_t> pList, QList<SuggestionThing *> & suggestionThingList, SuggestionThing::SuggestionThingType type, SketchWidget * sketchWidget) {
    int num = 0;
    foreach(QPairSD_t p, pList) {
        pairToSuggestionThing(p, suggestionThingList, type, sketchWidget);
        if (m_maxSuggestion < ++num) break;
        //DebugDialog::debug(QString("pair: %1, %2").arg(p.first).arg(p.second));
    }
}

void AutoCompleter::pairToSuggestionThing(QPairSD_t p, QList<SuggestionThing *> & suggestionThingList, SuggestionThing::SuggestionThingType type, SketchWidget *sketchWidget) {
	
	QStringList connectTo = p.first.split("_");
    SuggestionThing * suggestionT = new SuggestionThing();
    QString toID = getModuleIDByTitle(connectTo[0], sketchWidget);
    //DebugDialog::debug(QString("----- %1").arg(toID));

    //to pass paras;
    suggestionT->setType(type);
    suggestionT->addToModule(toID, connectTo[0], connectTo[1], *(new QList<QPairSS_t>()));
    suggestionT->setProbability(p.second);
    suggestionThingList.append(suggestionT);
}



QString AutoCompleter::getModuleID(QString title, SketchWidget * sketchWidget){
    QPointer<class ReferenceModel> referenceModel = sketchWidget->referenceModel();

    QList<ModelPart *> modelPartList = referenceModel->searchTitle(title, false);
    ModelPart * modelPart = NULL;

    //if (modelPartList[0]->title() == title) modelPart = modelPartList[0];

    foreach (ModelPart * mp, modelPartList) {
		if (mp->title() == title) {
			modelPart = mp;
			break;
        }
    }
    if (modelPart != NULL) {
        return modelPart->moduleID();
    } else {
        return findRestModuleID(title);
    }

}


QString AutoCompleter::findRestModuleID(QString title) {
	//if (title == "220Ω Resistor_connector0")
	DebugDialog::debug(QString("findRestModuleID: %1").arg(title));

    //Resistor
    QString ohmSymbol(QChar(0x03A9));
    QRegularExpression re(QString("^\\d.*%1 Resistor").arg(ohmSymbol));
    //QRegularExpression re("^\\d.*Resistor$");
	QRegularExpressionMatch match = re.match(title);
	if (match.hasMatch()) {
		DebugDialog::debug(QString("findRestModuleID:%1").arg(ModuleIDNames::ResistorModuleIDName));
		return ModuleIDNames::ResistorModuleIDName;
	}

	//LED
	re = QRegularExpression("\\w* \\(\\d*nm\\) LED");
    match = re.match(title);
	if (match.hasMatch()) {
		DebugDialog::debug(QString("findRestModuleID:%1").arg(ModuleIDNames::ResistorModuleIDName));
		return QString("5mmColor%1").arg(ModuleIDNames::LEDModuleIDName);
	}

	//TODO: IC

	//TODO: Generic Female/Male header 

	return " ";
}


