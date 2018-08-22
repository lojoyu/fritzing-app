#include "modelset.h"

#include "../debugdialog.h"

const QList<QString> MICROCONTROLLER({"Arduino Uno (Rev3)"});


typedef QPair<QString, QString> PairString;

ModelSet::ModelSet() {

    m_keyItem = NULL;
    m_keyid = -1;
    m_keyLabel = "";
    m_keyTitle = "";
    m_setid = -1;
    m_single = true;

}

ModelSet::ModelSet(long setid, QString title) {
    m_setid = setid;
    m_keyTitle = title;
    m_keyItem = NULL;
    m_keyid = -1;
    m_keyLabel = "";
    m_single = true;
    m_confirm = false;
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
    //m_terminalnameHash.insert(name, t);
    QList<Terminal> terminalList;
    if (m_terminalnameHash.contains(name)) {
        terminalList = m_terminalnameHash[name];
    }
    if (!terminalList.contains(t))
        terminalList.append(t);
    m_terminalnameHash.insert(name, terminalList);
    
}

void ModelSet::insertTerminalType(QString name, QString pintype) {
    m_terminalType.insert(name, pintype);
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

long ModelSet::setId() {
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
    QList<Terminal> tlist = m_terminalnameHash[terminalName];
    Terminal t = tlist[0];
    //Terminal t = m_terminalnameHash[terminalName];
    QString key = genLabelHashKey(t);
    if (!m_labelHash.contains(key)) {
        return QPair<ItemBase *, QString>(NULL, "");
    }
    ItemBase * item = m_labelHash[key];
    return QPair<ItemBase *, QString>(item, t.connectorID);
}

QList<QPair<ModelSet::Terminal, QString>> ModelSet::getPinTypeTerminal(QString pintype) {
    QList<QPair<Terminal, QString>> terminalList;

    foreach(QString terminalName, m_terminalType.keys()) {
        QString pinGet = pinEqual(pintype, m_terminalType[terminalName]);
        if (pinGet != "") {
            if (m_terminalnameHash.contains(terminalName)) {
                if (isMicrocontroller()) {
                    foreach(Terminal t, m_terminalnameHash[terminalName]) {
                        terminalList.append(QPair<ModelSet::Terminal, QString>(t, pinGet));
                    }
                } else {
                    terminalList.append(QPair<ModelSet::Terminal, QString>(m_terminalnameHash[terminalName][0], pinGet));
                }
            }
        }
    }
    return terminalList;    
}

QString ModelSet::pinEqual(QString pintype1, QString pintype2) {
    if (pintype2.contains(pintype1)) return pintype1;

    if (pintype1 == "VCC") {
        
        QRegularExpression re(QString("\\d.*V"));
        QRegularExpressionMatch match = re.match(pintype2);
        if (match.hasMatch()) return match.captured(0);
        
    } 
    return "";
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

QString ModelSet::keyLabel() {
    return m_keyLabel;
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

void ModelSet::addBreadboardConnection(QSharedPointer<SetConnection> s) {
    m_breadboardConnection = s;
}

QSharedPointer<SetConnection> ModelSet::setConnection() {
    return m_setConnection;
}

QSharedPointer<SetConnection> ModelSet::breadboardConnection() {
    return m_breadboardConnection;
}

void ModelSet::clearSetConnection() {
    m_setConnection.reset();
}

bool ModelSet::isConfirm() {
    return m_confirm;
}

void ModelSet::setConfirm() {
    m_confirm = true;
}

QList<QString> ModelSet::getConnectedTerminal() {
    QList<QString> connectedName;

    foreach(QSharedPointer<SetConnection> s, m_fromSetConnectionList) {
        if (!s->isConfirm()) continue;
        connectedName.append(s->getConnectedTerminal(0));
    }
    foreach(QSharedPointer<SetConnection> s, m_toSetConnectionList) {
        if (!s->isConfirm()) continue;
        connectedName.append(s->getConnectedTerminal(1));
    }
    return connectedName;
}

void ModelSet::appendSetConnectionList(int ind, QSharedPointer<SetConnection> setConnection) {
    if (ind == 0) m_fromSetConnectionList.append(setConnection);
    else if (ind == 1) m_toSetConnectionList.append(setConnection);
}


bool ModelSet::isMicrocontroller() {
    return MICROCONTROLLER.contains(m_keyTitle);
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
    Connection c(name1, name2);
    m_connectionList.append(c);
}

void SetConnection::appendConnection(QString name1, QString name2, QColor color) {
    Connection c(name1, name2, color, true);
    m_connectionList.append(c);
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

QList<SetConnection::Connection> SetConnection::getConnectionList() {
    return m_connectionList;

}

QSharedPointer<ModelSet> SetConnection::getFromModelSet() {
    return m_fromModelSet;
}

QSharedPointer<ModelSet> SetConnection::getToModelSet() {
    return m_toModelSet;
}

bool SetConnection::isConfirm() {
    return m_confirm;
}

void SetConnection::setConfirm() {
    m_confirm = true;
}

QList<QString> SetConnection::getConnectedTerminal(int ind) {
    QList<QString> connectedName;
    foreach(Connection c, m_connectionList) {
        if (ind == 0) connectedName.append(c.fromTerminal); 
        else if (ind == 1) connectedName.append(c.toTerminal);
    }
    return connectedName;
}

