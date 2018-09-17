#include "modelset.h"

#include "../debugdialog.h"

typedef QPair<QString, QString> PairString;

ModelSet::ModelSet() {

    m_keyItem = NULL;
    m_keyid = -1;
    m_keyLabel = "";
    m_keyTitle = "";
    m_setid = -1;
    m_single = true;
    m_keyModuleID = "";
    m_isMicrocontroller = false;

}

ModelSet::ModelSet(long setid, QString moduleID) {
    m_setid = setid;
    m_keyTitle = "";
    m_keyItem = NULL;
    m_keyid = -1;
    m_keyLabel = "";
    m_single = true;
    m_confirm = false;
    m_keyModuleID = moduleID;
    m_isMicrocontroller = false;
}

ModelSet::~ModelSet() {

    m_connections.clear();
    m_labelHash.clear();
    m_terminalHash.clear();
    m_terminalnameHash.clear();
    m_itemList.clear();
    m_wireConnection.clear();

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

void ModelSet::insertWireConnection(ItemBase* item, TerminalPair tp) {
    m_wireConnection.insert(item, tp);
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
    if (label == m_keyLabel) return m_keyItem;
    return NULL;
}

void ModelSet::addItem(ItemBase * item) {
    m_itemList.append(item);
}

QList<ItemBase *> ModelSet::getItemList() {
    return m_itemList;
}

QPair<ModelSet::Terminal, ModelSet::Terminal> ModelSet::getWireConnection(ItemBase* item) {
    if (m_wireConnection.contains(item)) {
        return m_wireConnection[item];
    }
    return QPair<Terminal, Terminal>();
}

long ModelSet::setId() {
    return m_setid;
}


void ModelSet::setKeyItem(ItemBase * item) {

    if (item == NULL) {
        m_keyid = -1;
        m_keyItem = NULL;
        return;
    }
    m_keyItem = item;
    insertLabelHash(m_keyLabel, item);
    m_keyid = item->id();
}

void ModelSet::setKeyId(long id) {
    m_keyid = id;
}

void ModelSet::setKeyLabel(QString label) {
    m_keyLabel = label;
}

void ModelSet::setKeyTitle(QString title) {
    m_keyTitle = title;
}

void ModelSet::setKeyModuleID(QString moduleID) {
    m_keyModuleID = moduleID;
}

void ModelSet::emptyItemList() {
    m_itemList.clear();
    m_labelHash.clear();
    m_wireConnection.clear();
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
        if (key == m_keyLabel) return QPair<ItemBase *, QString>(m_keyItem, t.connectorID);
        return QPair<ItemBase *, QString>(NULL, "");
    }
    ItemBase * item = m_labelHash[key];
    return QPair<ItemBase *, QString>(item, t.connectorID);
}

QList<QPair<ItemBase*, QString>> ModelSet::getItemAndCIDAll(QString terminalName) {
    if (!m_terminalnameHash.contains(terminalName)) {
        return QList<QPair<ItemBase *, QString>>();
    }
    QList<QPair<ItemBase *, QString>> getList;
    QList<Terminal> tlist = m_terminalnameHash[terminalName];
    foreach(Terminal t, tlist) {
        QString key = genLabelHashKey(t);
        if (!m_labelHash.contains(key)) {
            if (key == m_keyLabel) getList.append(QPair<ItemBase *, QString>(m_keyItem, t.connectorID));
            continue;
        }
        ItemBase * item = m_labelHash[key];
        getList.append(QPair<ItemBase*, QString> (item, t.connectorID));
    }

    return getList;
}

QString ModelSet::getConnectorID(QString terminalName) {
    if (m_terminalnameHash.contains(terminalName)) return m_terminalnameHash[terminalName][0].connectorID;
    else return "";
}


QString ModelSet::getPinType(QString connectorID) {
    if (m_terminalType.contains(connectorID)) {
        return m_terminalType[connectorID];
    }
    return "";
}

QList<QPair<ModelSet::Terminal, QString>> ModelSet::getPinTypeTerminal(QString pintype) {
    QList<QPair<Terminal, QString>> terminalList;
    QList<QPair<Terminal, QString>> terminalListLater;
    bool later = false;
    foreach(QString terminalName, m_terminalType.keys()) {
        // terminal type, terminal name
        QString pinGet = pinEqual(pintype, m_terminalType[terminalName]);
        if (pinGet != "") {
            later = false;
            if (pintype != m_terminalType[terminalName]) later = true;
            if (m_terminalnameHash.contains(terminalName)) {
                if (isMicrocontroller()) {
                    foreach(Terminal t, m_terminalnameHash[terminalName]) {
                        if (later) terminalListLater.append(QPair<ModelSet::Terminal, QString>(t, pinGet));
                        else terminalList.append(QPair<ModelSet::Terminal, QString>(t, pinGet));
                    }
                } else {
                    if (later) terminalListLater.append(QPair<ModelSet::Terminal, QString>(m_terminalnameHash[terminalName][0], pinGet));
                    else terminalList.append(QPair<ModelSet::Terminal, QString>(m_terminalnameHash[terminalName][0], pinGet));
                }
            }
        }
    }
    terminalList.append(terminalListLater);
    return terminalList;    
}

QString ModelSet::pinEqual(QString pintype1, QString pintype2) {

    if (pintype1 == "VCC") {

        QRegularExpression re(QString("\\d[^-\\s]*V"));
        QRegularExpressionMatch match = re.match(pintype2);
        if (match.hasMatch()) return match.captured(0);

    }

    if (pintype2.contains(pintype1)) return pintype1;


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

QString ModelSet::keyTitle() {
    return m_keyTitle;
}

QString ModelSet::keyModuleID() {
    return m_keyModuleID;
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
    setConfirm(true);
}

void ModelSet::setConfirm(bool confirm) {
    m_confirm = confirm;
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


//QString ModelSet::getPinType(QString connectorID) {
//    if (m_terminalType.contains(connectorID)) {
//        return m_terminalType[connectorID];
//    }
//    return "";
//}

/**********************
 * get connected setconnection with modelset
 * by searching setConnectionList
 * ********************/
QSharedPointer<SetConnection> ModelSet::getSetConnectionWithModelSet(QSharedPointer<ModelSet> modelset) {
    foreach(QSharedPointer<SetConnection> s, m_fromSetConnectionList) {
        if (s->getToModelSet() == modelset) {
            return s;
        }
    }
    foreach(QSharedPointer<SetConnection> s, m_toSetConnectionList) {
        if (s->getFromModelSet() == modelset) {
            return s;
        }
    }
    return QSharedPointer<SetConnection>(NULL);
}


/**********************
 * get connected terminal with modelset
 * by searching setConnectionList
 * @param:
 *  modelset: whose connections needs to be found out
 * @return:
 *  A list of pairs of terminal names.
 * ********************/
QList<QPair<QString, QString>> ModelSet::getConnectedPairWithModelSet(QSharedPointer<ModelSet> modelSet) {

    QList<QPair<QString, QString>> connectedPairList;
    foreach(QSharedPointer<SetConnection> s, m_fromSetConnectionList) {
        if (s->getToModelSet() == modelSet) {
            QList<SetConnection::Connection> connection = s->getConnectionList();
            foreach(SetConnection::Connection c, connection) {
                connectedPairList.append(QPair<QString, QString>(c.fromTerminal, c.toTerminal));
            }
        }
    }
    foreach(QSharedPointer<SetConnection> s, m_toSetConnectionList) {
        if (s->getFromModelSet() == modelSet) {
            QList<SetConnection::Connection> connection = s->getConnectionList();
            foreach(SetConnection::Connection c, connection) {
                connectedPairList.append(QPair<QString, QString>(c.fromTerminal, c.toTerminal));
            }
        }
    }
    return connectedPairList;
}

void ModelSet::setCount(int count) {
    m_count = count;
}

int ModelSet::count() {
    return m_count;
}



/**********************
 * get connected terminal with modelset
 * by searching setConnectionList
 * @param:
 *  modelset: whose connections needs to be found out
 * @return:
 *  A list of pairs of terminal names.
 * ********************/
QSet<QString> ModelSet::getConnectedNameWithModelSet(QSharedPointer<ModelSet> modelSet) {

    QSet<QString> connectedSet;
    foreach(QSharedPointer<SetConnection> s, m_fromSetConnectionList) {
        if (s->getToModelSet() == modelSet) {
            QList<SetConnection::Connection> connection = s->getConnectionList();
            foreach(SetConnection::Connection c, connection) {
                connectedSet.insert(c.fromTerminal);
            }
        }
    }
    foreach(QSharedPointer<SetConnection> s, m_toSetConnectionList) {
        if (s->getFromModelSet() == modelSet) {
            QList<SetConnection::Connection> connection = s->getConnectionList();
            foreach(SetConnection::Connection c, connection) {
                connectedSet.insert(c.toTerminal);
            }
        }
    }
    return connectedSet;
}


bool ModelSet::isMicrocontroller() {
    return m_isMicrocontroller;
}

/**************************
 * get terminal name matches moduleID & connectorID
 * by searching all terminals
  *************************/
QString ModelSet::getTerminalName(QString moduleID, QString connectorID) {
    foreach(QString name, m_terminalnameHash.keys()) {
        QList<Terminal> tlist = m_terminalnameHash[name];
        foreach(Terminal t, tlist) {
            if (t.moduleID == moduleID && t.connectorID == connectorID) return name;
        }
    }
    return "";
}




void ModelSet::deleteSetConnection(QSharedPointer<SetConnection> setConnection) {
    if (m_breadboardConnection == setConnection) m_breadboardConnection.clear();
    if (m_setConnection == setConnection) m_setConnection.clear();
    if (m_fromSetConnectionList.contains(setConnection)) m_fromSetConnectionList.removeOne(setConnection);
    if (m_toSetConnectionList.contains(setConnection)) m_toSetConnectionList.removeOne(setConnection);
}

ModelSet::Terminal ModelSet::findTerminal(long itemID, QString connectorID) {
    ItemBase * get = NULL;
    foreach(ItemBase * item, m_itemList) {
        if (item->id() == itemID) {
            get = item;
            break;
        }
    }
    if (!get) {
        if (m_keyid == itemID) get = m_keyItem;
    }

    foreach(TerminalPair tpair, m_connections) {
        Terminal t = tpair.first;
        if (m_labelHash[t.label] == get && t.connectorID == connectorID)
            return t;
        t = tpair.second;
        if (m_labelHash[t.label] == get && t.connectorID == connectorID)
            return t;
    }
    return Terminal();

}

ModelSet::Terminal ModelSet::getConnectedTerminal(ModelSet::Terminal t){
    foreach(TerminalPair tp, m_connections) {
        if (tp.first == t) return tp.second;
        if (tp.second == t) return tp.first;
    }
    return Terminal();
}

void ModelSet::setMicrocontroller() {
    m_isMicrocontroller = true;
}

///////

SetConnection::SetConnection() {
    
}

SetConnection::SetConnection(QSharedPointer<ModelSet> m1, QSharedPointer<ModelSet> m2) {
    m_fromModelSet = m1;
    m_toModelSet = m2;
    m_confirm = false;
}

SetConnection::~SetConnection() {

}

QSharedPointer<SetConnection> SetConnection::clone() {
    QSharedPointer<SetConnection> setconnection = QSharedPointer<SetConnection>(new SetConnection(m_fromModelSet, m_toModelSet));
    foreach(Connection c, m_connectionList) {
        setconnection->appendConnection(c.fromTerminal, c.toTerminal, c.color, c.changeColor);
    }
    return setconnection;
//    foreach(ItemBase * item, m_wireConnection.keys()) {
//        QPair<LongStringPair, LongStringPair> pair = m_wireConnection[item];
//        setconnection->insertWireConnection(item, pair);
//    }
}

void SetConnection::setModelSet(int ind, QSharedPointer<ModelSet> m) {
    if (ind == 0) m_fromModelSet = m;
    else m_toModelSet = m;
}

void SetConnection::appendConnection2(long id1, long id2) {
    m_connectionList2.append(QPair<long, long>(id1, id2));
}

void SetConnection::appendConnection(QString name1, QString name2) {
    Connection c(name1, name2);
    c.setCID(m_fromModelSet, m_toModelSet);
    m_connectionList.append(c);
}

void SetConnection::appendConnection(QString name1, QString name2, QColor color) {
    Connection c(name1, name2, color, true);
    c.setCID(m_fromModelSet, m_toModelSet);
    m_connectionList.append(c);
}

void SetConnection::appendConnection(QString name1, QString name2, QColor color, bool changeColor) {
    Connection c(name1, name2, color, changeColor);
    c.setCID(m_fromModelSet, m_toModelSet);
    m_connectionList.append(c);
}

bool SetConnection::compareConnectionFrom(const Connection &c1, const Connection &c2) {
 return c1.fromTerminal < c2.fromTerminal;
}

//bool SetConnection::compareConnectionTo(const Connection &c1, const Connection &c2) {
// return c1.toTerminal < c2.toTerminal;
//}

void SetConnection::sortConnectionList() {
    //qSort(m_connectionList.begin(), m_connectionList.end(), compareConnectionFrom);
    qSort(m_connectionList);
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
    setConfirm(true);
}

void SetConnection::setConfirm(bool confirm) {
    m_confirm = confirm;
}

QList<QString> SetConnection::getConnectedTerminal(int ind) {
    QList<QString> connectedName;
    foreach(Connection c, m_connectionList) {
        if (ind == 0) connectedName.append(c.fromTerminal); 
        else if (ind == 1) connectedName.append(c.toTerminal);
    }
    return connectedName;
}

void SetConnection::insertWireConnection(ItemBase * item, long itemID1, QString connectorID1, long itemID2, QString connectorID2) {
    QPair<long, QString> p1(itemID1, connectorID1);
    QPair<long, QString> p2(itemID2, connectorID2);
    m_wireConnection.insert(item, QPair<LongStringPair, LongStringPair>(p1, p2));
}

QPair<ModelSet::Terminal, ModelSet::Terminal> SetConnection::getWireConnection(ItemBase* item) {
    if (m_wireConnection.contains(item)) {
        QPair<LongStringPair, LongStringPair> pair = m_wireConnection[item];
        ModelSet::Terminal t1 = m_fromModelSet->findTerminal(pair.first.first, pair.first.second);
        ModelSet::Terminal t2 = m_toModelSet->findTerminal(pair.second.first, pair.second.second);
        return QPair<ModelSet::Terminal, ModelSet::Terminal>(t1, t2);
    }
    return QPair<ModelSet::Terminal, ModelSet::Terminal>();

}

// ind: from whom
QString SetConnection::getConnectedTo(int ind, QString name) {
    foreach(Connection c, m_connectionList) {
        if (ind == 0 && c.fromTerminal == name) return c.toTerminal;
        if (ind == 1 && c.toTerminal == name) return c.fromTerminal;
    }

    return "";
}
