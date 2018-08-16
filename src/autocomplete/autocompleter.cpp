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
    m_sketchwidget = NULL;
}

AutoCompleter::~AutoCompleter() {

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

void AutoCompleter::getSuggestionSetSelf(ItemBase * item, SketchWidget * sketchWidget) {
    QString title = item->title();
    QList<QMap<QString, QVariant> *> resultList = AutocompleteDBManager::getModelSet(title);
    QList<QSharedPointer<ModelSet>> modelSetList;
    mapListToModelSet(item, sketchWidget, resultList, modelSetList);
    qDeleteAll(resultList);
    resultList.clear();
    if (modelSetList.length() > 0) {
        if (modelSetList.length() == 1 && modelSetList[0]->single()){
            getSuggestionNextSelf(modelSetList[0], sketchWidget);
        } else {
            sketchWidget->addModelSet(modelSetList[0], true);
        }
    }
    //getSuggestionNextSelf(modelSetList[0], sketchWidget);
    //return modelSetList;
}

void AutoCompleter::mapListToModelSet(ItemBase * keyItem, SketchWidget * sketchWidget, QList<QMap<QString, QVariant> *> & resultList, QList<QSharedPointer<ModelSet>> & modelSetList) {
    QString title = keyItem == NULL ? "" : keyItem->title();
    long setid = -1;
    QSharedPointer<ModelSet> modelSet;
    for (int i=0; i<resultList.length(); i++) {
        QMap<QString, QVariant> map = *resultList[i];
        long nowid = map["module_id"].toLongLong();
        if (setid == -1 || setid != nowid) {
            setid = nowid;
            modelSet = QSharedPointer<ModelSet>(new ModelSet(nowid, title));
            DebugDialog::debug(QString("%2~~~~~~~~~~~~~~~%1").arg(modelSet->keyId()).arg(setid));
            modelSetList.append(modelSet);
            modelSet->setSingle(true);
        }
        //TODO: Handling NULL
        QString title1 = map["component_title"].toString();
        QString title2 = map["to_component_title"].toString();
        
        QString m1 = getModuleIDByTitle(title1, sketchWidget);
        QString m2 = getModuleIDByTitle(title2, sketchWidget);
        ModelSet::Terminal t1 = ModelSet::Terminal(m1, title1, map["component_label"].toString(), map["component_terminal"].toString());
        ModelSet::Terminal t2 = ModelSet::Terminal(m2, title2, map["to_component_label"].toString(), map["to_component_terminal"].toString());
        modelSet->appendConnection(t1, t2);
        modelSet->insertTerminalHash(map["id"].toLongLong(), t1);
        modelSet->insertTerminalnameHash(map["name"].toString(), t1);
        if (title1 == title) {
            modelSet->setKeyLabel(map["component_label"].toString());
            modelSet->setKeyItem(keyItem);
            //keyItem->setModelSet(modelSet);
        }
        if (title2 != "NULL") modelSet->setSingle(false);
    }
}

//TODO: cannot handle set-set with different connection
void AutoCompleter::mapListToSetConnection(QSharedPointer<ModelSet> modelset, QList<QSharedPointer<ModelSet>> & toModelsetList, QList<QMap<QString, QVariant> *> & connectionList, QList<QSharedPointer<SetConnection>> & setConnectionList){
    int ind = 0;
    long cid = -1;
    QSharedPointer<SetConnection> setconnection;
    for (int i=0; i<connectionList.length(); i++) {
        QMap<QString, QVariant> map = *connectionList[i];
        long nowid = map["connection_id"].toLongLong();
        if (cid == -1 || cid != nowid) {
            cid = nowid;
            setconnection = QSharedPointer<SetConnection>(new SetConnection(modelset, toModelsetList[ind]));
            //DebugDialog::debug(QString("mapListToSetConnection: %1").arg(modelset->keyItem()->title()));
            //DebugDialog::debug(QString("mapListToSetConnection: %1").arg(toModelsetList[ind]->title()));
            setConnectionList.append(setconnection);
            ind++;
        }
        //setconnection->appendConnection(map["terminal_id"].toLongLong(), map["to_terminal_id"].toLongLong());
        setconnection->appendConnection(map["terminal_id"].toString(), map["to_terminal_id"].toString());
    }
}

void AutoCompleter::expandModelSetList(QList<long> moduleList, QList<QSharedPointer<ModelSet>> & toModelsetList) {
    DebugDialog::debug(QString("autocompleter::expandModelSetList %1").arg(toModelsetList.length()));
    QHash<long, QSharedPointer<ModelSet>> idhash;
    for (int i=0; i<toModelsetList.length(); i++) {
        QSharedPointer<ModelSet> modelset = toModelsetList[i];
        idhash.insert(modelset->getSetId(), modelset);
    }
    toModelsetList.clear();
    for (int i=0; i<moduleList.length(); i++) {
        toModelsetList.append(idhash[moduleList[i]]);
    }
}

void AutoCompleter::getSuggestionNextSelf(QSharedPointer<ModelSet> modelset, SketchWidget * sketchWidget) {
//QList<ModelSet *> AutoCompleter::getSuggestionNextSelf(long setid, SketchWidget * sketchWidget) {
    //TODO: check if no need to frequent ask db
    //DebugDialog::debug(QString("getSuggestionNext: %1").arg(modelset->keyItem()->title()));
    long setid = modelset->getSetId();
    QList<QPair<long, long>> toList = AutocompleteDBManager::getFrequentConnect(setid, m_maxSuggestion);
    if (toList.length() == 0) return;
    QList<long> idList;
    QList<long> moduleList;
    foreach(LongLongPair pair, toList) {
        idList.append(pair.first);
        moduleList.append(pair.second);
    }
    QList<QMap<QString, QVariant> *> connectionList = AutocompleteDBManager::getConnectionsByID(idList);
    QList<QMap<QString, QVariant> *> modelsetMapList = AutocompleteDBManager::getModelSetsByID(moduleList);
    QList<QSharedPointer<ModelSet>> toModelsetList;
    QList<QSharedPointer<SetConnection>> setConnectionList;
    mapListToModelSet(NULL, sketchWidget, modelsetMapList, toModelsetList);
    expandModelSetList(moduleList, toModelsetList);
    mapListToSetConnection(modelset, toModelsetList, connectionList, setConnectionList);
    qDeleteAll(connectionList);
    qDeleteAll(modelsetMapList);
    connectionList.clear();
    modelsetMapList.clear();

    sketchWidget->addSetToSet(toModelsetList[0], setConnectionList[0], true);

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

