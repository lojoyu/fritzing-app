#include "modelset.h"


ModelSet::ModelSet(long setid, QString title) {
	m_setid = setid;
	m_title = title;
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

QList<ItemBase*> ModelSet::getItemList() {
	return m_itemList;
}

long ModelSet::getSetId() {
	return m_setid;
}


void ModelSet::setKeyItem(ItemBase* item) {
	m_keyItem = item;
}

void ModelSet::emptyItemList() {
	m_itemList.clear();
}

QPair<ItemBase *, QString> ModelSet::getItemAndCID(long terminalId) {
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

QString ModelSet::genLabelHashKey(Terminal t) {
	return QString("%1%2").arg(t.title).arg(t.label);
}

ItemBase * ModelSet::keyItem() {
    return m_keyItem;
}

///////

SetConnection::SetConnection(ModelSet * m1, ModelSet * m2) {
	m_fromModelSet = m1;
	m_toModelSet = m2;
}

SetConnection::~SetConnection() {

}

void SetConnection::appendConnection(long id1, long id2) {
    m_connectionList.append(QPair<long, long>(id1, id2));
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

QList<QPair<long, long>> SetConnection::getConnectionList() {
	return m_connectionList;

}

ModelSet * SetConnection::getFromModelSet() {
    return m_fromModelSet;
}

ModelSet * SetConnection::getToModelSet() {
    return m_toModelSet;
}
