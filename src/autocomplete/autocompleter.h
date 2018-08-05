#ifndef AUTOCOMPLETER_H
#define AUTOCOMPLETER_H


#include <QVariant>

#include "../sketch/sketchwidget.h"
#include "suggestion.h"


class AutoCompleter
{
typedef QPair<QString, double> QPairSD_t;
typedef QPair<QString, QString> QPairSS_t;


public:
	AutoCompleter();
	~AutoCompleter();
	static void test();
	static void setDatasheetMap(QVariantMap &);
	static void setHistoryMap(QVariantMap &);
    static void setTerminalMap(QVariantMap &);
    static void getSuggestions(long fromID, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget);
    static QList<Suggestion *> getSuggestions(long fromID, SketchWidget * sketchWidget);
    static void getSuggestions(long fromID, QString fromConnectorID, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget);
    static Suggestion * getSuggestions(long fromID, QString fromConnectorID, SketchWidget * sketchWidget);
    static QString getModuleIDByTitle(QString title);
    static void setMaxSuggestion(int max);

protected:
	void setDatasheetMapSelf(QVariantMap & qVariantMap);
	void setHistoryMapSelf(QVariantMap & qVariantMap);
	void setTerminalMapSelf(QVariantMap &);
    void getSuggestionsSelf(long fromID, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget);
	QList<Suggestion *> getSuggestionsSelf(long fromID, SketchWidget * sketchWidget);
	Suggestion * getSuggestionsSelf(long fromID, QString fromConnectorID, SketchWidget * sketchWidget);
	void getSuggestionsSelf(long fromID, QString fromConnectorID, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget);
	QString getModuleID(QString title, SketchWidget * sketchWidget);
    QString getModuleIDByTitle(QString title, SketchWidget * sketchWidget);
	void sortTerminalPairMap(QVariantMap & toSortMap);
	void sortTerminalTriMap(QVariantMap & toSortMap);
	static bool pairSecondComparer(const QPairSD_t &p1, const QPairSD_t &p2);
	QString getModuleIDByTitleSelf(QString title);
	void saveToHash(QString title);
	void pairsToSuggestionThings(QList<QPairSD_t> pList, QList<SuggestionThing*> & suggestionThingList, SuggestionThing::SuggestionThingType type, SketchWidget * sketchWidget);
	void pairToSuggestionThing(QPairSD_t p, QList<SuggestionThing*> & suggestionThingList, SuggestionThing::SuggestionThingType type, SketchWidget * sketchWidget);
	void setMaxSuggestionSelf(int max);
    QString findRestModuleID(QString title);
    //void getSuggestionsFromHistory();
    void sortHistoryMap();
    void getSuggestionsParallel(Suggestion * suggestion, QList<Suggestion*> & suggestionList, SketchWidget * sketchWidget);
    void getSuggestionsFromHistory(long fromID, Suggestion * suggestion, QList<Suggestion *> & suggestionList,SketchWidget * sketchWidget);
    void getSuggestionsFromDatasheet(long fromID, Suggestion * fromSuggestion, QList<Suggestion *> & suggestionList,SketchWidget * sketchWidget);
    void historyToSuggestions(QString connectName, long fromID, QList<Suggestion *> & suggestionList, SketchWidget * sketchWidget);
    Suggestion * findFromSuggestionList(long fromID, QString fromConnectorID, QList<Suggestion *> & suggestionList);

protected:
	static AutoCompleter* singleton;
	QVariantMap m_terminalPairMap;
	QVariantMap m_terminalTriMap;
	QVariantMap m_historyMap;
	QVariantMap m_datasheetMap;
	QHash<QString, QString> m_titleToModuleID;
	int m_maxSuggestion = 5;
	//QList<QPairSS_t> m_temp_paras
};

#endif // AUTOCOMPLETER_H
