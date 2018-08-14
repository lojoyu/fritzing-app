#ifndef AUTOCOMPLETER_H
#define AUTOCOMPLETER_H

#include <QVariant>
#include <QObject>

#include "../sketch/sketchwidget.h"
#include "modelset.h"
#include "../fritzing-app/src/mainwindow/mainwindow.h"


class AutoCompleter : public QObject
{
	
typedef QPair<QString, double> QPairSD_t;
typedef QPair<QString, QString> QPairSS_t;

public:
	AutoCompleter();
	~AutoCompleter();
	//new
    static AutoCompleter* getAutoCompleter();
    static void setDatabasePath(const QString & databasename);
    static void getSuggestionSet(ItemBase * item, SketchWidget * sketchWidget);
    static void getSuggestionNext(ModelSet * modelset, SketchWidget * sketchWidget);
	static void test();

signals:
    void NextSelf_signal(QList<ModelSet *> , QList<SetConnection *>);
    void SetSelf_signal(QList<ModelSet *>);

protected:
    void getSuggestionSetSelf(ItemBase * item, SketchWidget * sketchWidget);
    void getSuggestionNextSelf(ModelSet * modelset, SketchWidget * sketchWidget);
	QString getModuleID(QString title, SketchWidget * sketchWidget);
    QString getModuleIDByTitle(QString title, SketchWidget * sketchWidget);
    QString findRestModuleID(QString title);
    QList<ModelSet *> mapListToModelSet(ItemBase * keyItem, SketchWidget * sketchWidget, QList<QMap<QString, QVariant> *> resultList);
    QList<SetConnection *> mapListToSetConnection(ModelSet * modelset, QList<ModelSet *> toModelsetList, SketchWidget * sketchWidget, QList<QMap<QString, QVariant> *>connectionList);

    void addModelSet(ModelSet *);
    
protected:
	//new
    int m_maxSuggestion = 5;
	static AutoCompleter* singleton;
	class SketchWidget* m_sketchwidget;
    QHash<QString, QString> m_titleToModuleID;
    class MainWindow * mw;

	
};

#endif // AUTOCOMPLETER_H
