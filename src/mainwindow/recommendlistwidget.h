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
#include <QNetworkReply>
#include <QDomDocument>
#include <QNetworkAccessManager>

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
    RecommendListWidget(SketchWidget * sketchWidget, QPointer<ReferenceModel> referenceModel,
                        QListWidget * tutorialList, QWidget *parent = 0);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void execDrag();

public slots:

    void onItemEnteredSlot(QListWidgetItem*);
    void onItemClickedSlot(QListWidgetItem*);

    void setTosetList(QList<QSharedPointer<ModelSet>> toModelsetList, QList<QSharedPointer<SetConnection>> setConnectionList, QList<QList<QString> *> tutorialLink);
    void setModelSetList(QList<QSharedPointer<ModelSet>> modelSetList);
    void clearList();

    void itemClickedTimeout();
    void onItemdDoubleClickedSlot(QListWidgetItem*);
    void getTutorialMessage(QNetworkReply *);
    void getTutorial_img(QNetworkReply *);

protected:

    void onItemEvent(QListWidgetItem*, bool);
    void loadImage(ModelPart * modelPart, QListWidgetItem * lwi);
    void readTutorial(const QDomDocument &);

protected:
    SketchWidget * m_sketchwidget;
    QPointer<ReferenceModel> m_referenceModel;
    QPoint m_startPoint;
    QListWidget * m_tutorial;
    bool mDoubleClicked;
    QNetworkAccessManager * manager = new QNetworkAccessManager(this);
    QString cleanData(const QString &);
    QString Tutorial_title;
    QString Tutorial_img;
    QNetworkAccessManager * img_manager = new QNetworkAccessManager(this);
    QByteArray img_bytes ;
    QPixmap pixmap;
    bool waitForMessage;
    bool waitForImg;
    QNetworkReply *netReply;
    QEventLoop loop;

};

#endif
