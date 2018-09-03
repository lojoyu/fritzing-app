#ifndef RECOMMENDLISTWIDGET_H
#define RECOMMENDLISTWIDGET_H

#include <QListWidget>
#include <QApplication>
#include <QPoint>
#include <QMouseEvent>
#include <QMimeData>
#include <QDrag>
#include <QListWidgetItem>
#include <QIcon>

#include "../sketch/sketchwidget.h"
#include "../autocomplete/modelset.h"
#include "../model/modelpart.h"


//class MainWindow;
class RecommendListWidget : public QListWidget {
    Q_OBJECT

public:
    enum SuggestionType {
        toModelSet,
        setToSet
    };
    RecommendListWidget(SketchWidget * sketchWidget, QPointer<ReferenceModel> referenceModel, QWidget *parent = 0);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void execDrag();

public slots:

    void onItemEnteredSlot(QListWidgetItem*);
    void onItemClickedSlot(QListWidgetItem*);

    void setTosetList(QList<QSharedPointer<ModelSet>> toModelsetList, QList<QSharedPointer<SetConnection>> setConnectionList);
    void setModelSetList(QList<QSharedPointer<ModelSet>> modelSetList);
    void clearList();

protected:

    void onItemEvent(QListWidgetItem*, bool);
    void loadImage(ModelPart * modelPart, QListWidgetItem * lwi);

protected:
    SketchWidget * m_sketchwidget;
    QPointer<ReferenceModel> m_referenceModel;
    QPoint m_startPoint;

};

#endif
