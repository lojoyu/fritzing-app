#ifndef AUTOCOMPLETER_H
#define AUTOCOMPLETER_H

#include <QVariant>
#include <QObject>

#include "../sketch/sketchwidget.h"
#include "modelset.h"


class AutoCompleter : public QObject
{
	
typedef QPair<QString, double> QPairSD_t;
typedef QPair<QString, QString> QPairSS_t;

public:
	AutoCompleter();
	~AutoCompleter();
	//new
    static void setDatabasePath(const QString & databasename);
    static void getSuggestionSet(ItemBase * item, SketchWidget * sketchWidget);
    static void getSuggestionNext(QSharedPointer<ModelSet> modelset, SketchWidget * sketchWidget);
	static void test();

protected:
    void getSuggestionSetSelf(ItemBase * item, SketchWidget * sketchWidget);
    void getSuggestionNextSelf(QSharedPointer<ModelSet> modelset, SketchWidget * sketchWidget);
	QString getModuleID(QString title, SketchWidget * sketchWidget);
    QString getModuleIDByTitle(QString title, SketchWidget * sketchWidget);
    QString findRestModuleID(QString title);
    void mapListToModelSet(ItemBase * keyItem, SketchWidget * sketchWidget, QList<QMap<QString, QVariant> *> & resultList, QList<QSharedPointer<ModelSet>> & modelSetList);
    void mapListToSetConnection(QSharedPointer<ModelSet> modelset, QList<QSharedPointer<ModelSet>> & toModelsetList, QList<QMap<QString, QVariant> *> & connectionList, QList<QSharedPointer<SetConnection>> & setConnectionList);
    void expandModelSetList(QList<long> moduleList, QList<QSharedPointer<ModelSet>> & toModelsetList);
    void addModelSet(QSharedPointer<ModelSet>);
    
protected:
	//new
    int m_maxSuggestion = 5;
	static AutoCompleter* singleton;
	class SketchWidget* m_sketchwidget;
    QHash<QString, QString> m_titleToModuleID;
	
};

#endif // AUTOCOMPLETER_H
