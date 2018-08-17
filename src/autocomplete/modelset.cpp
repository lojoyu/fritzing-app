#include "modelset.h"

#include "../debugdialog.h"

ModelSet::ModelSet() {

    m_keyItem = NULL;
    m_keyid = -1;
    m_keyLabel = "";
    m_title = "";
    m_setid = -1;
    m_single = true;

}

ModelSet::ModelSet(long setid, QString title) {
    m_setid = setid;
    m_title = title;
    m_keyItem = NULL;
    m_keyid = -1;
    m_keyLabel = "";
    m_title = "";
    m_single = true;
}

ModelSet::~ModelSet() {

    m_connections.clear();
    m_labelHash.clear();
    m_terminalHash.clear();
    m_terminalnameHash.clear();
    m_itemList.clear();

}

void ModelSet::appendConnection(Terminal t1, Terminal t2) {
    m_connections.append(TerminalPair(t1, t2));
}

void ModelSet::insertLabelHash(QString label, ItemBase * item){
    m_labelHash.insert(label, item);
}

void ModelSet::insertTerminalHash(long id, Terminal t) {
    m_terminalHash.insert(id, t);
}

void ModelSet::insertTerminalnameHash(QString name, Terminal t) {
    m_terminalnameHash.insert(name, t);
}

QList<ModelSet::TerminalPair> ModelSet::getConnections() {
    return m_connections;
}

ItemBase * ModelSet::getItem(QString label) {
    if (m_labelHash.contains(label)) return m_labelHash[label];
    return NULL;
}

void ModelSet::addItem(ItemBase * item) {
    m_itemList.append(item);
}

QList<ItemBase *> ModelSet::getItemList() {
    return m_itemList;
}

long ModelSet::getSetId() {
    return m_setid;
}


void ModelSet::setKeyItem(ItemBase * item) {
    insertLabelHash(m_keyLabel, item);
    m_keyItem = item;
    m_keyid = item->id();
}

void ModelSet::setKeyId(long id) {
    m_keyid = id;
}

void ModelSet::setKeyLabel(QString label) {
    m_keyLabel = label;
}

void ModelSet::emptyItemList() {
    m_itemList.clear();
    m_labelHash.clear();
}

QPair<ItemBase *, QString> ModelSet::getItemAndCID2(long terminalId) {
    if (!m_terminalHash.contains(terminalId)) {
        return QPair<ItemBase *, QString>(NULL, "");
    }
    Terminal t = m_terminalHash[terminalId];
    QString key = genLabelHashKey(t);
    if (!m_labelHash.contains(key)) {
        return QPair<ItemBase *, QString>(NULL, "");
    }
    ItemBase * item = m_labelHash[key];
    return QPair<ItemBase *, QString>(item, t.connectorID);
}

QPair<ItemBase *, QString> ModelSet::getItemAndCID(QString terminalName) {
    if (!m_terminalnameHash.contains(terminalName)) {
        return QPair<ItemBase *, QString>(NULL, "");
    }
    Terminal t = m_terminalnameHash[terminalName];
    QString key = genLabelHashKey(t);
    if (!m_labelHash.contains(key)) {
        return QPair<ItemBase *, QString>(NULL, "");
    }
    ItemBase * item = m_labelHash[key];
    return QPair<ItemBase *, QString>(item, t.connectorID);
}

QString ModelSet::genLabelHashKey(Terminal t) {
    //return QString("%1%2").arg(t.title).arg(t.label);
    return t.label;
}

ItemBase * ModelSet::keyItem() {
    return m_keyItem;
}

qint64 ModelSet::keyId() {
    return m_keyid;
}

void ModelSet::setSingle(bool b) {
    m_single = b;
}

bool ModelSet::single() {
    return m_single;
}

void ModelSet::addSetConnection(QSharedPointer<SetConnection> s) {
    m_setConnection = s;
}

QSharedPointer<SetConnection> ModelSet::setConnection() {
    return m_setConnection;
}

void ModelSet::clearSetConnection() {
    m_setConnection.reset();
}


///////

SetConnection::SetConnection() {
}

SetConnection::SetConnection(QSharedPointer<ModelSet> m1, QSharedPointer<ModelSet> m2) {
    m_fromModelSet = m1;
    m_toModelSet = m2;
}

SetConnection::~SetConnection() {

}

void SetConnection::appendConnection2(long id1, long id2) {
    m_connectionList2.append(QPair<long, long>(id1, id2));
}

void SetConnection::appendConnection(QString name1, QString name2) {
    m_connectionList.append(QPair<QString, QString>(name1, name2));
}

QList<ItemBase *> SetConnection::getWireList() {
    return m_wireList;
}

void SetConnection::appendWireList(ItemBase * item){
    m_wireList.append(item);
}

void SetConnection::emptyWireList() {
    m_wireList.clear();
}

QList<QPair<long, long>> SetConnection::getConnectionList2() {
    return m_connectionList2;

}

QList<QPair<QString, QString>> SetConnection::getConnectionList() {
    return m_connectionList;

}

QSharedPointer<ModelSet> SetConnection::getFromModelSet() {
    return m_fromModelSet;
}

QSharedPointer<ModelSet> SetConnection::getToModelSet() {
    return m_toModelSet;
}
