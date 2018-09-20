#ifndef MODELSET_H
#define MODELSET_H

#include <QtCore>
#include <QSharedPointer>
#include "../items/wire.h"

class ItemBase;
class SetConnection;

class ModelSet {

public:
	/*
	enum TerminalType{
		Gnd,
		Vcc,
        V33,
        V5,
        V9,
        V12,
		Digital,
        Analog,
        Unknown
	};*/

	struct Terminal {
		QString moduleID;
        QString title;
		QString label;
		QString connectorID;
		QString type;
        QString name;

        Terminal():Terminal("", "", "", "", "", "") {};
        Terminal(QString mid, QString t, QString l, QString cid, QString ty, QString n) {
			moduleID = mid;
			title = t;
			label = l;
			connectorID = cid;
            type = ty;
            name = n;
		} 
        QString toString() const {
            return moduleID+title+label+connectorID+type;
		}

        QString myString() const {
            return moduleID+title+label+connectorID+type;
        }

		bool operator==(const Terminal & other ) const {
            return this->myString()==other.myString();
		}


	};
    typedef QPair<Terminal, Terminal> TerminalPair;

public:
	ModelSet();
	ModelSet(long setid, QString moduleID);
    ~ModelSet();
	void appendConnection(Terminal t1, Terminal t2);
    void insertLabelHash(QString label, ItemBase * item);
	void insertTerminalHash(long id, Terminal t);
	void insertTerminalnameHash(QString name, Terminal t);
	void insertTerminalType(QString name, QString type);

	QList<TerminalPair> getConnections();
	ItemBase * getItem(QString label);
	void addItem(ItemBase * item);
	QList<ItemBase *> getItemList();
    void setKeyItem(ItemBase * item);
	void setKeyLabel(QString label);
    void setKeyId(long id);
    void setKeyTitle(QString title);
    void setKeyModuleID(QString moduleID);
	void emptyItemList();
	long setId();
	QPair<ItemBase *, QString> getItemAndCID2(long terminalId);
	QPair<ItemBase *, QString> getItemAndCID(QString terminalname);
    QList<QPair<ItemBase*, QString>> getItemAndCIDAll(QString terminalName);
    QString getConnectorID(QString terminalName);
	QString genLabelHashKey(Terminal t);
    ItemBase * keyItem();
    void setSingle(bool b);
    bool single(); 
    qint64 keyId();
    QString keyLabel();
    QString keyTitle();
    QString keyModuleID();
    void addSetConnection(QSharedPointer<SetConnection> s);
    void addBreadboardConnection(QSharedPointer<SetConnection> s);
    QSharedPointer<SetConnection> setConnection();
    QSharedPointer<SetConnection> breadboardConnection();
    void clearSetConnection();
    bool isConfirm();
    void setConfirm();
    void setConfirm(bool confirm);
    QList<QString> getConnectedTerminal();
    void appendSetConnectionList(int ind, QSharedPointer<SetConnection> setConnection);
    bool isMicrocontroller();
    QList<QPair<Terminal, QString>> getPinTypeTerminal(QString pintype);
    static QString pinEqual(QString pintype1, QString pintype2);
    QString getTerminalName(QString moduleID, QString connectorID);
    void deleteSetConnection(QSharedPointer<SetConnection> setConnection);
    void insertWireConnection(ItemBase* item, TerminalPair tp);
    TerminalPair getWireConnection(ItemBase* item);
    Terminal findTerminal(long itemID, QString connectorID);
    QString getPinType(QString connectorID);
    Terminal getConnectedTerminal(Terminal t);
    void setMicrocontroller();
    QSharedPointer<SetConnection> getSetConnectionWithModelSet(QSharedPointer<ModelSet> modelset);
    QList<QPair<QString, QString>> getConnectedPairWithModelSet(QSharedPointer<ModelSet> modelSet);
    QList<QPair<QString, QString>> getConnectedPair();
    QSet<QString> getConnectedNameWithModelSet(QSharedPointer<ModelSet> modelSet);
    void setCount(int count);
    int count();
    //void setSupportModuleID(QString supportModuleID);
    //QString supportModuleID();


protected:
	//static long m_nextid;
    ItemBase * m_keyItem;
    //Item * item; // or id
    QString m_keyTitle; //keyTitle
	QString m_keyLabel;
	qint64 m_keyid;
	QString m_keyModuleID;
	long m_setid; //from db
	QList<TerminalPair> m_connections;
    QList<ItemBase *> m_itemList;
    QHash<QString, ItemBase *> m_labelHash;
    QHash<long, Terminal> m_terminalHash;
    //QHash<QString, Terminal> m_terminalnameHash;
    QHash<QString, QList<Terminal>> m_terminalnameHash;
    QHash<QString, QString> m_terminalType; //terminalname, type
    bool m_single;
    QList<QSharedPointer<SetConnection>> m_fromSetConnectionList;
    QList<QSharedPointer<SetConnection>> m_toSetConnectionList;
    QSharedPointer<SetConnection> m_setConnection;
    QSharedPointer<SetConnection> m_breadboardConnection;
    bool m_confirm;
    QHash<ItemBase *, TerminalPair> m_wireConnection;
    bool m_isMicrocontroller;
    int m_count;
    //QString m_supprotModuleID;

};


class SetConnection {
    typedef QPair<long, QString> LongStringPair;
public:
	struct Connection {
		QString fromTerminal;
        QString toTerminal;
		QColor color;
		bool changeColor;
        QString fromCID;
        QString toCID;

		Connection():Connection("", "") {};
		Connection(QString fromT, QString toT):Connection(fromT, toT, QColor(0, 0, 0), false) {};
		Connection(QString fromT, QString toT, QColor c, bool b) {
			fromTerminal = fromT;
			toTerminal = toT;
			color = c;
			changeColor = b;
		}

        void setFromCID(QSharedPointer<ModelSet> from) {
            fromCID = from->getConnectorID(fromTerminal);
        }
        void setToCID(QSharedPointer<ModelSet> to) {
            toCID = to->getConnectorID(toTerminal);
        }

        void setCID(QSharedPointer<ModelSet> from, QSharedPointer<ModelSet> to) {
            setFromCID(from);
            setToCID(to);
        }

        bool operator<(const Connection& other) const {
            if (fromCID.length() < other.fromCID.length()) return true;
            else if (fromCID.length() > other.fromCID.length()) return false;
            if (fromCID < other.fromCID) return true;
            if (fromCID == other.fromCID) {
                if (toCID.length() < other.toCID.length()) return true;
                return toCID < other.toCID;
            }
            return false;
        }
	};

	SetConnection();
	SetConnection(QSharedPointer<ModelSet> m1, QSharedPointer<ModelSet> m2);
	~SetConnection();
    QSharedPointer<SetConnection> clone();
	void appendConnection2(long id1, long id2);
	void appendConnection(QString name1, QString name2);
	void appendConnection(QString name1, QString name2, QColor color);
    void appendConnection(QString name1, QString name2, QColor color, bool changeColor);
	QSharedPointer<ModelSet> getFromModelSet();
	QSharedPointer<ModelSet> getToModelSet();
	QList<ItemBase *> getWireList();
	void appendWireList(ItemBase *);
	void emptyWireList();
	QList<QPair<long, long>> getConnectionList2();
	QList<QPair<QString, QString>> getConnectionList1();
	QList<Connection> getConnectionList();
	bool isConfirm();
    void setConfirm();
    void setConfirm(bool confirm);
    QList<QString> getConnectedTerminal(int ind);
    void insertWireConnection(ItemBase * item, long itemID1, QString connectorID1, long itemID2, QString connectorID2);
    QPair<ModelSet::Terminal, ModelSet::Terminal> getWireConnection(ItemBase* item);
    void setModelSet(int ind, QSharedPointer<ModelSet> m);
    QString getConnectedTo(int ind, QString name);
    void sortConnectionList();
    static bool compareConnectionFrom(const Connection &c1, const Connection &c2);

protected:
	QSharedPointer<ModelSet> m_fromModelSet;
	QSharedPointer<ModelSet> m_toModelSet;
	//QHash<long, long> m_connectionHash;
	QList<QPair<long, long>> m_connectionList2;
	QList<QPair<QString, QString>> m_connectionList1;
	QList<Connection> m_connectionList;
	QList<ItemBase *> m_wireList;
	bool m_confirm;
    QHash<ItemBase *,  QPair<LongStringPair, LongStringPair>> m_wireConnection;
};

Q_DECLARE_METATYPE(ModelSet)
Q_DECLARE_METATYPE(SetConnection)

Q_DECLARE_METATYPE(QSharedPointer<ModelSet>)
Q_DECLARE_METATYPE(QSharedPointer<SetConnection>)

#endif
