#ifndef MODELSET_H
#define MODELSET_H

#include <QtCore>
#include "../items/wire.h"

class ItemBase;
class ModelSet {

public:
	struct Terminal {
		QString moduleID;
        QString title;
		QString label;
		QString connectorID;
		Terminal() {};
		Terminal(QString mid, QString t, QString l, QString cid) {
			moduleID = mid;
			title = t;
			label = l;
			connectorID = cid;
		}
	};
    typedef QPair<Terminal, Terminal> TerminalPair;

public:
	ModelSet(long setid, QString title);
	void appendConnection(Terminal t1, Terminal t2);
	void insertLabelHash(QString label, ItemBase * item);
	void insertTerminalHash(long id, Terminal t);
	QList<TerminalPair> getConnections();
	ItemBase * getItem(QString label);
	void addItem(ItemBase * item);
	QList<ItemBase*> getItemList();
	void setKeyItem(ItemBase* item);
	void emptyItemList();
	long getSetId();
	QPair<ItemBase *, QString> getItemAndCID(long terminalId);
	QString genLabelHashKey(Terminal t);
    ItemBase * keyItem();

protected:
	static long m_nextid;
    ItemBase * m_keyItem = NULL;
    //Item * item; // or id
	QString m_title;
	long m_setid; //from db
	QList<TerminalPair> m_connections;
    QList<ItemBase*> m_itemList;
    QHash<QString, ItemBase *> m_labelHash;
    QHash<long, Terminal> m_terminalHash;

};


class SetConnection {

public:
	SetConnection(ModelSet * m1, ModelSet * m2);
	~SetConnection();
	void appendConnection(long id1, long id2);
	ModelSet * getFromModelSet();
	ModelSet * getToModelSet();
	QList<ItemBase *> getWireList();
	void appendWireList(ItemBase *);
	void emptyWireList();
	QList<QPair<long, long>> getConnectionList();

protected:
	ModelSet * m_fromModelSet;
	ModelSet * m_toModelSet;
	//QHash<long, long> m_connectionHash;
	QList<QPair<long, long>> m_connectionList;
	QList<ItemBase * > m_wireList;


};

#endif
