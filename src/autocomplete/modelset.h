#ifndef MODELSET_H
#define MODELSET_H

#include <QtCore>
#include <QSharedPointer>
#include "../items/wire.h"

class ItemBase;
class SetConnection;

class ModelSet {

public:
	struct Terminal {
		QString moduleID;
        QString title;
		QString label;
		QString connectorID;
		Terminal():Terminal("", "", "", "") {};
		Terminal(QString mid, QString t, QString l, QString cid) {
			moduleID = mid;
			title = t;
			label = l;
			connectorID = cid;
		}
	};
    typedef QPair<Terminal, Terminal> TerminalPair;

public:
	ModelSet();
	ModelSet(long setid, QString title);
    ~ModelSet();
	void appendConnection(Terminal t1, Terminal t2);
    void insertLabelHash(QString label, ItemBase * item);
	void insertTerminalHash(long id, Terminal t);
	void insertTerminalnameHash(QString name, Terminal t);
	QList<TerminalPair> getConnections();
	ItemBase * getItem(QString label);
	void addItem(ItemBase * item);
	QList<ItemBase *> getItemList();
    void setKeyItem(ItemBase * item);
	void setKeyLabel(QString label);
    void setKeyId(long id);
	void emptyItemList();
	long getSetId();
	QPair<ItemBase *, QString> getItemAndCID2(long terminalId);
	QPair<ItemBase *, QString> getItemAndCID(QString terminalname);
	QString genLabelHashKey(Terminal t);
    ItemBase * keyItem();
    void setSingle(bool b);
    bool single(); 
    qint64 keyId();
    void addSetConnection(QSharedPointer<SetConnection> s);
    QSharedPointer<SetConnection> setConnection();
    void clearSetConnection();

protected:
	//static long m_nextid;
    ItemBase * m_keyItem;
    //Item * item; // or id
	QString m_title;
	QString m_keyLabel;
	qint64 m_keyid;
	long m_setid; //from db
	QList<TerminalPair> m_connections;
    QList<ItemBase *> m_itemList;
    QHash<QString, ItemBase *> m_labelHash;
    QHash<long, Terminal> m_terminalHash;
    QHash<QString, Terminal> m_terminalnameHash;
    bool m_single;
    QSharedPointer<SetConnection> m_setConnection;

};


class SetConnection {

public:
	SetConnection();
	SetConnection(QSharedPointer<ModelSet> m1, QSharedPointer<ModelSet> m2);
	~SetConnection();
	void appendConnection2(long id1, long id2);
	void appendConnection(QString name1, QString name2);
	QSharedPointer<ModelSet> getFromModelSet();
	QSharedPointer<ModelSet> getToModelSet();
	QList<ItemBase *> getWireList();
	void appendWireList(ItemBase *);
	void emptyWireList();
	QList<QPair<long, long>> getConnectionList2();
	QList<QPair<QString, QString>> getConnectionList();

protected:
	QSharedPointer<ModelSet> m_fromModelSet;
	QSharedPointer<ModelSet> m_toModelSet;
	//QHash<long, long> m_connectionHash;
	QList<QPair<long, long>> m_connectionList2;
	QList<QPair<QString, QString>> m_connectionList;
	QList<ItemBase *> m_wireList;
};

Q_DECLARE_METATYPE(ModelSet)
Q_DECLARE_METATYPE(SetConnection)

Q_DECLARE_METATYPE(QSharedPointer<ModelSet>)
Q_DECLARE_METATYPE(QSharedPointer<SetConnection>)

#endif
