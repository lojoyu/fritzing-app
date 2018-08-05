#ifndef SUGGESTION_H
#define SUGGESTION_H

#include <QHash>
#include "../items/itembase.h"


class SuggestionThing {

typedef QPair<QString, double> QPairSD_t;
typedef QPair<QString, QString> QPairSS_t;


public:
    enum SuggestionThingType {
        SuggestionThingUnkown,
        SuggestionThingFromDataSheet,
        SuggestionThingSingle,
        SuggestionThingParallel,
        SuggestionThingFromHistory
    };
	struct ToModule {
        ItemBase * itemBase;
		QString moduleID;
		QString title;
		QString connectorID;
		QList<QPairSS_t> paras;
        //FOR DATASHEET TYPE
        QString label;
	};   
    //SuggestionThing(long toID, QString toTitle, QString toConnectorID, double probability);
    //SuggestionThing(QList<long> & toIDList, QList<QString> & toConnectorIDList, double prabability);
    SuggestionThing();
    //SuggestionThing(ToModule toModule);
    //SuggestionThing(QList<ToModule> toModules);
    ~SuggestionThing();
    ToModule * addToModule(QString moduleID, QString title, QString connectorID, QList<QPairSS_t> & paras);
    ToModule * addToModule(QString moduleID, QString title, QString connectorID);
   	QList<ToModule *> getToModuleList();
    void setProbability(double p);
    const double getProbability();
    void appendUpdateHash(ToModule *, ToModule *);
    void addUpdateHash(ToModule *, QList<ToModule *>);  
    QString getDatasheetTitle();
    SuggestionThingType getType();
    QString getTypeString();
    void setType(SuggestionThingType);
    void setDatasheetTitle(QString datasheetTitle);

protected:
	void updateItemBase();

public:
    double m_probability;

protected:
    QList<ToModule *> m_toModuleList;
    //QList<long> m_toIDList;
    //QList<QString> m_toIDTitle;
    //QList<QString> m_toConnectorIDList;
    //double m_probability;
    SuggestionThingType m_type = SuggestionThingUnkown;
    //QHash<ToModule *, QList<ToModule *>> m_itemUpdateHash;
    //FOR DATASHEET TYPE
    //QStringList m_datasheetPath;
    QString m_datasheetTitle;


};
/*
class SuggestionSet {

public:
	SuggestionSet();
	~SuggestionSet();
protected:

protected:


};*/


class Suggestion {

typedef QPair<QString, double> QPairSD_t;

public:

	Suggestion(long fromID, QString fromTitle, QString fromConnectorID, QList<SuggestionThing *> suggestionList);
	~Suggestion();
	
	//void setSTList(QList<QPairSD_t> &);
	//void appendSTList(QList<QPairSD_t> &);
	void appendSTList(QList<SuggestionThing *>);
    long getFromID();
    QString getFromTitle();
    QString getFromConnectorID();
    SuggestionThing* getSTNext();
    SuggestionThing* getSTNow();
    QString getSTNowID();
    QString getSTNowConnectorID();
    QString getSTNowTitle();
    static bool probComparer(const SuggestionThing *s1, const SuggestionThing *s2);
    static bool testComparer(const int &s1, const int &s2);


protected:

    

protected:
	//QLIST?
	long m_fromID;
	QString m_fromConnectorID;
    QString m_fromTitle;
    QList<SuggestionThing *> m_suggestionThingList;
   	int m_STInd = -1;


};

#endif // SUGGESTION_H
