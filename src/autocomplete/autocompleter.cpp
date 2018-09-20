#include "autocompleter.h"

#include "../debugdialog.h"
#include "autocompletedbmanager.h"
#include "modelset.h"

#include "../sketch/sketchwidget.h"
#include "../referencemodel/referencemodel.h"
#include "../model/modelpart.h"
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

typedef QPair<long, long> LongLongPair;
typedef QMap<QString, QVariant> StringVairantMap;

void debugPrintResult(QString str, QList<QMap<QString, QVariant> *> resultList) {
    DebugDialog::debug(str);
    foreach(StringVairantMap * resultMap, resultList) {
        DebugDialog::debug("-----");
        foreach(QString k, resultMap->keys()) {
            DebugDialog::debug(QString("%1-> %2").arg(resultMap->value(k).toString()).arg(k));
        }
    }
}


AutoCompleter* AutoCompleter::singleton = NULL;

AutoCompleter::AutoCompleter() {

}

AutoCompleter::~AutoCompleter() {

}

AutoCompleter * AutoCompleter::getAutoCompleter() {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    return singleton;
}

void AutoCompleter::setDatabasePath(const QString & databasename) {
    AutocompleteDBManager::loadDB(databasename);
}

void AutoCompleter::test() {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    //singleton->getSuggestionNextSelf(2, NULL);
}

//static
void AutoCompleter::getSuggestionSet(ItemBase * item, SketchWidget * sketchWidget) {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    singleton->getSuggestionSetSelf(item, sketchWidget);
}

void AutoCompleter::getSuggestionNext(QSharedPointer<ModelSet> modelset, SketchWidget * sketchWidget) {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    singleton->getSuggestionNextSelf(modelset, sketchWidget);
}

void AutoCompleter::getSuggestionExist(QSharedPointer<ModelSet> modelset, SketchWidget * sketchWidget) {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    singleton->getSuggestionExistSelf(modelset, sketchWidget);
}

void AutoCompleter::getSuggestionConnection(QSharedPointer<ModelSet> fromModelset, ConnectorItem * fromConnectorItem,
    QSharedPointer<ModelSet> toModelset, ConnectorItem * toConnectorItem, SketchWidget * sketchWidget) {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    singleton->getSuggestionConnectionSelf(fromModelset, fromConnectorItem, toModelset, toConnectorItem, sketchWidget);

}

void AutoCompleter::getSuggestionConnection(QSharedPointer<ModelSet> fromModelset, QSharedPointer<ModelSet> toModelset,
                             QList<QPair<QString, QString>> stringpair, SketchWidget * sketchWidget) {

    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    singleton->getSuggestionConnectionSelf(fromModelset, toModelset, stringpair, sketchWidget);
}

void AutoCompleter::clearRecommend() {
    if (singleton == NULL) {
        singleton = new AutoCompleter();
    }
    singleton->clearRecommendSelf();
}

void AutoCompleter::clearRecommendSelf() {
    emit clearRecommendListSignal();
}


void AutoCompleter::getSuggestionSetSelf(ItemBase * item, SketchWidget * sketchWidget) {

    QString moduleID = item->moduleID();
    QList<QMap<QString, QVariant> *> resultList = AutocompleteDBManager::getModelSet(moduleID);
    QList<QSharedPointer<ModelSet>> modelSetList; // add count
    mapListToModelSet(item, sketchWidget, resultList, modelSetList);
    qDeleteAll(resultList);
    resultList.clear();
    QList<double> percentageList;
    double total = 0;
    foreach(QSharedPointer<ModelSet> ms, modelSetList) {
        total += ms->count();
    }
    foreach(QSharedPointer<ModelSet> ms, modelSetList) {
        percentageList.append(ms->count()/total);
    }
    if (modelSetList.length() > 0) {

        emit addModelSetSignal(modelSetList, percentageList);
        // if (modelSetList.length() == 1 && modelSetList[0]->single()){
        //     sketchWidget->selectModelSet(modelSetList[0], true);
        //     //emit addModelSetSignal(modelSetList);
        //     //modelSetList[0]->setKeyItem(item);
        //     //sketchWidget->completeVoltage(modelSetList[0]);
        //     //completeVoltage(modelSetList[0]);
        //     //getSuggestionNextSelf(modelSetList[0], sketchWidget);
        // } else {
        //     emit addModelSetSignal(modelSetList);
        //     //sketchWidget->addModelSet(modelSetList[0], true);
        // }
    } else {
        clearRecommendSelf();
    }
}

void AutoCompleter::mapListToModelSet(ItemBase * keyItem, SketchWidget * sketchWidget, QList<QMap<QString, QVariant> *> & resultList, QList<QSharedPointer<ModelSet>> & modelSetList) {
   // QString title = keyItem == NULL ? "" : keyItem->title();
    QString moduleID = keyItem == NULL ? "" : keyItem->moduleID();
    long setid = -1;
    QSharedPointer<ModelSet> modelSet;
    for (int i=0; i<resultList.length(); i++) {
        QMap<QString, QVariant> map = *resultList[i];
        long nowid = map["module_id"].toLongLong();
        if (setid == -1 || setid != nowid) {
            if (keyItem == NULL) {
                moduleID = map["module_fid"].toString();
            }
            setid = nowid;
            modelSet = QSharedPointer<ModelSet>(new ModelSet(nowid, moduleID));
            if (map["mcu_or_bat"] == "Y") modelSet->setMicrocontroller();
            // if there is microcontroller on the breadboard, use that one
            if (modelSet->isMicrocontroller()) {
                QList<QSharedPointer<ModelSet>> msList = sketchWidget->getMicrocontroller();
                foreach(QSharedPointer<ModelSet> ms, msList) {
                    if (!ms.isNull() && modelSet->keyModuleID() == ms->keyModuleID()) {
                        modelSet = ms;
                        break;
                    }
                }

            }
            modelSetList.append(modelSet);
            modelSet->setSingle(true);
            modelSet->setCount(map["count"].toInt());
        }
        //TODO: Handling NULL
        QString title1 = map["component_title"].toString();
        QString title2 = map["to_component_title"].toString();
        
        //TODO: change to moduleid -> categorize!
        QString m1 = map["component_module_fid"].toString();
        QString m2 = map["to_component_module_fid"].toString();

        ModelSet::Terminal t1 = ModelSet::Terminal(m1, title1, map["component_label"].toString(), map["component_terminal"].toString(), map["type"].toString(), map["name"].toString());
        ModelSet::Terminal t2 = ModelSet::Terminal(m2, title2, map["to_component_label"].toString(), map["to_component_terminal"].toString(), map["type"].toString(), map["name"].toString());

        modelSet->appendConnection(t1, t2);

        modelSet->insertTerminalHash(map["id"].toLongLong(), t1);
        modelSet->insertTerminalnameHash(map["name"].toString(), t1);
        if (title2 == "NULL") {
            t2.name = "";
            t2.type = "";
        } else {
            modelSet->insertTerminalnameHash(map["name"].toString(), t2);
        }
        modelSet->insertTerminalType(map["name"].toString(), map["type"].toString());
        if (m1 == moduleID) {
            modelSet->setKeyModuleID(moduleID);
            modelSet->setKeyTitle(title1);
            modelSet->setKeyLabel(map["component_label"].toString());
            //modelSet->setKeyItem(keyItem);
            if (keyItem != NULL) modelSet->setKeyId(keyItem->id());
            //keyItem->setModelSet(modelSet);
        }
        if (m2 != "NULL") modelSet->setSingle(false);
    }
}

QList<long> AutoCompleter::mapListToSetConnection(QSharedPointer<ModelSet> modelset, QList<QSharedPointer<ModelSet>> & toModelsetList,
    QList<QMap<QString, QVariant> *> & connectionList, QList<QSharedPointer<SetConnection>> & setConnectionList, bool inorder){
    int ind = 0;
    long cid = -1;
    QHash<long, QSharedPointer<ModelSet>> modelSetHash;
    if (!inorder) {
        foreach(QSharedPointer<ModelSet> m, toModelsetList) {
            modelSetHash.insert(m->setId(), m);
        }
        toModelsetList.clear();
    }
    QList<long> idList;
    QSharedPointer<SetConnection> setconnection;
    QSharedPointer<ModelSet> nowModelSet;
    for (int i=0; i<connectionList.length(); i++) {
        QMap<QString, QVariant> map = *connectionList[i];
        long nowid = map["connection_id"].toLongLong();
        if (cid == -1 || cid != nowid) {
            cid = nowid;
            idList.append(nowid);
            if (!inorder) {
                long mid = map["to_module_id"].toLongLong();
                nowModelSet = modelSetHash[mid];
                toModelsetList.append(modelSetHash[mid]);
            }
            else {
                nowModelSet = toModelsetList[ind];
            } 
            setconnection = QSharedPointer<SetConnection>(new SetConnection(modelset, nowModelSet));
            setConnectionList.append(setconnection);
            ind++;
        }
        QString t1 = map["terminal_id"].toString();
        QString t2 = map["to_terminal_id"].toString();
        QString type = nowModelSet->getPinType(map["to_terminal_id"].toString());
        if (type != "") {
            if (ModelSet::pinEqual("VCC", type) != "") {
                setconnection->appendConnection(t1, t2, QColor(255, 0, 0));
            } else if (ModelSet::pinEqual("GND", type) != "") {
                setconnection->appendConnection(t1, t2, QColor(0, 0, 0));
            } else {
                setconnection->appendConnection(t1, t2);
            }
        } else {
            setconnection->appendConnection(t1, t2);
        }

    }
    foreach(QSharedPointer<SetConnection> sc, setConnectionList) {
        sc->sortConnectionList();
    }
    return idList;
}

void AutoCompleter::expandModelSetList(QList<long> moduleList, QList<QSharedPointer<ModelSet>> & toModelsetList) {
    QHash<long, QSharedPointer<ModelSet>> idhash;
    for (int i=0; i<toModelsetList.length(); i++) {
        QSharedPointer<ModelSet> modelset = toModelsetList[i];
        idhash.insert(modelset->setId(), modelset);
    }
    toModelsetList.clear();
    for (int i=0; i<moduleList.length(); i++) {
        toModelsetList.append(idhash[moduleList[i]]);
    }
}

void AutoCompleter::getSuggestionNextSelf(QSharedPointer<ModelSet> modelset, SketchWidget * sketchWidget) {
    QList<QSharedPointer<ModelSet>> toModelsetList;
    QList<QSharedPointer<SetConnection>> setConnectionList;

    QList<QString> nameList = modelset->getConnectedTerminal();

    long setid = modelset->setId();
    QList<QPair<long, long>> toList = AutocompleteDBManager::getFrequentConnect(setid, m_maxSuggestion, nameList);
    if (toList.length() == 0) { // what?
        emit addSetConnectionSignal(toModelsetList, setConnectionList, QList<QList<QString> *>(), QList<double>(), false);
        return;
    }
    QList<long> idList;
    QList<long> moduleList;
    foreach(LongLongPair pair, toList) {
        idList.append(pair.first);
        moduleList.append(pair.second);
    }
    //QList<QMap<QString, QVariant> *> connectionList = AutocompleteDBManager::getConnectionsByID(idList);
    QList<QMap<QString, QVariant> *> modelsetMapList = AutocompleteDBManager::getModelSetsByID(moduleList);
    QList<QList<QString> *> tutorialList = AutocompleteDBManager::getTutorialList(idList, 20, true);
    QList<int> countList = AutocompleteDBManager::getCountFromConnections(idList, true, false, nameList);
    mapListToModelSet(NULL, sketchWidget, modelsetMapList, toModelsetList);
    //expandModelSetList(moduleList, toModelsetList);
    //mapListToSetConnection(modelset, toModelsetList, connectionList, setConnectionList, true);
    //qDeleteAll(connectionList);
    qDeleteAll(modelsetMapList);
    QList<double> percentageList;
    foreach(int c, countList) {
        percentageList.append((double)c/(double)modelset->count());
    }
    foreach(QSharedPointer<ModelSet> ms, toModelsetList) {
        QSharedPointer<SetConnection> sc = QSharedPointer<SetConnection>(new SetConnection(modelset, ms));
        setConnectionList.append(sc);
    }

    //connectionList.clear();
    modelsetMapList.clear();
    //TODO: count star: modelsetF+modelsetT / modelsetF
    emit addSetConnectionSignal(toModelsetList, setConnectionList, tutorialList, percentageList, false);
   // mw->setTosetList(toModelsetList, setConnectionList);
    //sketchWidget->addSetToSet(toModelsetList[0], setConnectionList[0], true);

}

void AutoCompleter::getSuggestionExistSelf(QSharedPointer<ModelSet> modelset, SketchWidget * sketchWidget) {
    QList<QSharedPointer<ModelSet>> existModelset = sketchWidget->getSavedModelSets();
    QHash<long, QSharedPointer<ModelSet>> modelSetHash;
    QList<long> mids;
    foreach(QSharedPointer<ModelSet> m, existModelset) {

        mids.append(m->setId());
        modelSetHash.insert(m->setId(), m);
    }
    QList<QMap<QString, QVariant> *> connectionList = AutocompleteDBManager::getConnectionsByModuleID(modelset->setId(), mids);
    QList<QSharedPointer<SetConnection>> setConnectionList;
    mapListToSetConnection(modelset, existModelset, connectionList, setConnectionList, false);

    qDeleteAll(connectionList);
    connectionList.clear();
    //emit addSetConnectionSignal(existModelset, setConnectionList, QList<QList<QString>*>(), false);

}

//void AutoCompleter::getSuggestionConnectionSelf(QSharedPointer<ModelSet> fromModelset,
//        QSharedPointer<ModelSet> toModelset, SketchWidget * sketchWidget) {

//    QList<QPair<QString, QString>> connectedPair = fromModelset->getConnectedPairWithModelSet(toModelset);
//    getSuggestionConnectionSelf(fromModelset, toModelset, connectedPair, sketchWidget);
//}


void AutoCompleter::getSuggestionConnectionSelf(QSharedPointer<ModelSet> fromModelset, ConnectorItem * fromConnectorItem,
        QSharedPointer<ModelSet> toModelset, ConnectorItem * toConnectorItem, SketchWidget * sketchWidget) {

    QList<QPair<QString, QString>> stringpair;
    if (fromConnectorItem != NULL && toConnectorItem != NULL) {
        QString fromConnectorID = fromConnectorItem->connectorSharedID();
        QString toConnectorID = toConnectorItem->connectorSharedID();
        QString fromName = fromModelset->getTerminalName(fromConnectorItem->attachedTo()->moduleID(), fromConnectorID);
        QString toName = toModelset->getTerminalName(toConnectorItem->attachedTo()->moduleID(), toConnectorID);

        stringpair.append(QPair<QString, QString>(fromName, toName));
    }
    //QList<QPair<QString, QString>> connectedPair = fromModelset->getConnectedPairWithModelSet(toModelset);
    getSuggestionConnectionSelf(fromModelset, toModelset, stringpair, sketchWidget);
}

void AutoCompleter::getSuggestionConnectionSelf(QSharedPointer<ModelSet> fromModelset, QSharedPointer<ModelSet> toModelset,
        QList<QPair<QString, QString>> stringpair, SketchWidget * sketchWidget) {

    QList<QMap<QString, QVariant> *> connectionList = AutocompleteDBManager::getConnectionsBetweenModules(fromModelset->setId(), toModelset->setId(), stringpair);
    QList<QSharedPointer<SetConnection>> setConnectionList;
    QList<QSharedPointer<ModelSet>> existModelset;
    existModelset.append(toModelset);
    QList<long> ids = mapListToSetConnection(fromModelset, existModelset, connectionList, setConnectionList, false);
    QList<QList<QString> *> tutorialList = AutocompleteDBManager::getTutorialList(ids, 20);
    QList<int> countList = AutocompleteDBManager::getCountFromConnections(ids, false, false);
    QList<double> percentageList;

    qDeleteAll(connectionList);
    connectionList.clear();
    double total = 0;
    foreach(int c, countList) {
        total += c;
    }
    foreach(int c, countList) {
        percentageList.append((double)c/(double)total);
    }

    if (setConnectionList.length() > 0) {
        //sketchWidget->addSetToSet(toModelset, setConnectionList[0], false, true);
        //TODO: count star: connection/modelSet+modelSet
        emit addSetConnectionSignal(existModelset, setConnectionList, tutorialList, percentageList, true);
    }
}


QString AutoCompleter::getModuleIDByTitle(QString title, SketchWidget * sketchWidget) {
    if (m_titleToModuleID.contains(title)) return m_titleToModuleID[title];
    else {
        QString toModuleID = getModuleID(title, sketchWidget);
        m_titleToModuleID.insert(title, toModuleID);
        return toModuleID;
    }
}

QString AutoCompleter::getModuleID(QString title, SketchWidget * sketchWidget){
    QPointer<class ReferenceModel> referenceModel = sketchWidget->referenceModel();

    QList<ModelPart *> modelPartList = referenceModel->searchTitle(title, false);
    ModelPart * modelPart = NULL;
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

    //Resistor
    QString ohmSymbol(QChar(0x03A9));
    QRegularExpression re(QString("^\\d.*%1 Resistor").arg(ohmSymbol));
    //QRegularExpression re("^\\d.*Resistor$");
    QRegularExpressionMatch match = re.match(title);
    if (match.hasMatch()) {
        return ModuleIDNames::ResistorModuleIDName;
    }

    //LED
    re = QRegularExpression("\\w* \\(\\d*nm\\) LED");
    match = re.match(title);
    if (match.hasMatch()) {
        return QString("5mmColor%1").arg(ModuleIDNames::LEDModuleIDName);
    }

    //TODO: IC



    //TODO: Generic Female/Male header 

    return " ";
}

