#include "suggestion.h"
#include "../debugdialog.h"

#include <QList>

/*
SuggestionThing::SuggestionThing(long toID, QString toConnectorID, double probability) {
    QList<long> toIDList();
    QList<toConnectorID> toConnectorIDList();
    toIDList.append(toID);
    toConnectorIDList.append(toConnectorID);

    SuggestionThing(toIDList, toConnectorIDList, probability);
}

SuggestionThing::SuggestionThing(QList<long> & toIDList, QList<QString> & toConnectorIDList, double prabability) {
    m_toIDList = toIDList;
    m_toConnectorIDList = toConnectorIDList;
    m_probability = probability;
}*/
SuggestionThing::SuggestionThing() {

}

SuggestionThing::~SuggestionThing() {

}

SuggestionThing::ToModule * SuggestionThing::addToModule(QString moduleID, QString title, QString connectorID, QList<QPairSS_t> & paras){
    ToModule * toModule = new ToModule();
    toModule->moduleID = moduleID;
    toModule->title = title;
    toModule->connectorID = connectorID;
    toModule->paras = paras;
    m_toModuleList.append(toModule);
    return toModule;
}

SuggestionThing::ToModule * SuggestionThing::addToModule(QString moduleID, QString title, QString connectorID){
    ToModule * toModule = new ToModule();
    toModule->moduleID = moduleID;
    toModule->title = title;
    toModule->connectorID = connectorID;
    DebugDialog::debug(QString("toModule:%1").arg(toModule->title));
    //toModule.paras = QList<QPairSS_t>
    m_toModuleList.append(toModule);
    return toModule;
}

QList<SuggestionThing::ToModule *> SuggestionThing::getToModuleList() {
    return m_toModuleList;
}

void SuggestionThing::setProbability(double p) {
    m_probability = p;
}

const double SuggestionThing::getProbability() {
    return m_probability;
}

QString SuggestionThing::getDatasheetTitle() {
    return m_datasheetTitle;
}

SuggestionThing::SuggestionThingType SuggestionThing::getType() {
    return m_type;
}

void SuggestionThing::setType(SuggestionThing::SuggestionThingType type) {
    m_type = type;
}

QString SuggestionThing::getTypeString() {
    QString str;
    switch (m_type) {
    	case SuggestionThingUnkown:
    		str = "Unkown";
    		break;
    	case SuggestionThingFromDataSheet:
    		str = "From datasheet";
    		break;
    	case SuggestionThingSingle:
    		str = "Single";
    		break;
    	case SuggestionThingParallel:
    		str = "Parallel";
    		break;
    	case SuggestionThingFromHistory:
    		str = "History";
    		break;
    }
    return str;
}

void SuggestionThing::setDatasheetTitle(QString datasheetTitle){
    m_datasheetTitle = datasheetTitle;
}

/*
QString SuggestionThing::getModuleID() {
    return m_toModule
}*/

/*
void SuggestionThing::appendUpdateHash(ToModule * keyToModule, ToModule * toAppend) {
    QList<ToModule *> toModuleList;
    if (m_itemUpdateHash.contains(keyToModule)) {
        toModuleList = m_itemUpdateHash[keyToModule];
    }
    toModuleList.append(toAppend);
    m_itemUpdateHash.insert(keyToModule, toModuleList);
}

void SuggestionThing::addUpdateHash(ToModule * keyToModule, QList<ToModule *> toModuleList) {
    m_itemUpdateHash.insert(keyToModule, toModuleList);
}*/

///////////////////

Suggestion::Suggestion(long fromID, QString fromTitle, QString fromConnectorID, QList<SuggestionThing *> suggestionThingList) {
    m_fromID = fromID;
    m_fromTitle = fromTitle;
    m_fromConnectorID = fromConnectorID;
    m_suggestionThingList = suggestionThingList;
}

Suggestion::~Suggestion() {

}

long Suggestion::getFromID() {
    return m_fromID;
}

QString Suggestion::getFromConnectorID() {
    return m_fromConnectorID;
}

QString Suggestion::getFromTitle() {
    return m_fromTitle;
}
/*
void Suggestion::setSTList(QList<QPairSD_t> & probPairList) {
    //clear
    appendSuggestionThingList(probPairList);
}
//TODO?
void Suggestion::appendSTList(QList<QPairSD_t> & probPairList) {
    //sort
    foreach(QPairSD_t p, probPairList) {
        //m_suggestionThingList.append(QPairToSuggestionThing(p));
    }
}*/

void Suggestion::appendSTList(QList<SuggestionThing *> suggestionThingList) {
    //TODO: check if can merge, sort by probability
    m_suggestionThingList.append(suggestionThingList);

    qSort(m_suggestionThingList.begin(), m_suggestionThingList.end(), probComparer);

}

bool Suggestion::testComparer(const int &s1, const int &s2) {
    return s1 > s2;
}

bool Suggestion::probComparer(const SuggestionThing *s1, const SuggestionThing *s2) {
    //return s1.m_probability > s2.m_probability;
    return s1->m_probability > s2->m_probability;
    //return (s1.getProbability()) > (s2.getProbability());
}

SuggestionThing* Suggestion::getSTNext() {
    if (m_suggestionThingList.count() <= (m_STInd+1)) return NULL;

    m_STInd++;
    return m_suggestionThingList.at(m_STInd);
}

SuggestionThing* Suggestion::getSTNow() {
    return m_suggestionThingList.at(m_STInd);
}

QString Suggestion::getSTNowID() {
    return getSTNow()->getToModuleList().at(0)->moduleID;
}

QString Suggestion::getSTNowConnectorID() {
    return getSTNow()->getToModuleList().at(0)->connectorID;
}


QString Suggestion::getSTNowTitle() {
    return getSTNow()->getToModuleList().at(0)->title;
}




