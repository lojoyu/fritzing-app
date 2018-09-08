#include "recommendlistwidget.h"

#include <QSizeGrip>
#include <QStatusBar>
#include <QtDebug>
#include <QApplication>
#include <QListWidget>
#include <QLabel>
#include <QHBoxLayout>


#include "../items/partfactory.h"
#include "../layerattributes.h"
#include "../fsvgrenderer.h"
#include "../infoview/htmlinfoview.h"
#include "../debugdialog.h"

RecommendListWidget::RecommendListWidget(SketchWidget * sketchWidget, QPointer<ReferenceModel> referenceModel, QListWidget * tutorialList, QWidget *parent) {
	m_sketchwidget = sketchWidget;
    m_referenceModel = referenceModel;
    m_tutorial = tutorialList;

    //QNetworkAccessManager * manager = new QNetworkAccessManager(this);
    //connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(getTutorialMessage(QNetworkReply *)));
    //connect(img_manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(getTutorial_img(QNetworkReply *)));
    //manager->get(QNetworkRequest(QUrl("http://fritzing.org/projects/ardpicprog-arduino-shield")));
    //connect(netReply, SIGNAL(finished()), &loop, SLOT(quit()));


	connect(this, SIGNAL(itemEntered(QListWidgetItem*)),
                       this, SLOT(onItemEnteredSlot(QListWidgetItem*)));

    connect(this, SIGNAL(itemClicked(QListWidgetItem*)),
                this, SLOT(onItemClickedSlot(QListWidgetItem*)));

    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                this, SLOT(onItemdDoubleClickedSlot(QListWidgetItem*)));


    AutoCompleter * autocompleter = AutoCompleter::getAutoCompleter();

    connect(autocompleter, SIGNAL(addModelSetSignal(QList<QSharedPointer<ModelSet>>)),
                       this, SLOT(setModelSetList(QList<QSharedPointer<ModelSet>>)));
    connect(autocompleter, SIGNAL(addSetConnectionSignal(QList<QSharedPointer<ModelSet>>, QList<QSharedPointer<SetConnection>>, QList<QList<QString> *>)),
                       this, SLOT(setTosetList(QList<QSharedPointer<ModelSet>>, QList<QSharedPointer<SetConnection>>, QList<QList<QString> *>))) ;

    connect(autocompleter, SIGNAL(clearRecommendListSignal()), this, SLOT(clearList()));
}

void RecommendListWidget::getTutorial_img(QNetworkReply * networkReply){
    QNetworkAccessManager * manager = networkReply->manager();
    int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    DebugDialog::debug("getTutorial_img-----------------1");
    if (responseCode == 200) {
        img_bytes = networkReply->readAll();
        DebugDialog::debug("getTutorial_img-----------------2");
        pixmap.loadFromData(img_bytes);
        DebugDialog::debug("getTutorial_img-----------------3");
    }

    manager->deleteLater();
    manager->clearAccessCache();
    networkReply->deleteLater();
    networkReply->close();

}

QString RecommendListWidget::cleanData(const QString & data) {
    //static QRegExp ListItemMatcher("<img src=.*.png\"");
    static QRegExp ListItemMatcher("<img src=\".*\"");
    ListItemMatcher.setMinimal(true);           // equivalent of lazy matcher

    QDomDocument doc;
    QStringList listItems;
    int pos = 0;

    static QRegExp divMatcher("<div>.*</div>");
    divMatcher.setMinimal(true);
    while (pos < data.count()) {
        int ix = data.indexOf(divMatcher, pos);
        if (ix < 0) break;

        QString listItem = divMatcher.cap(0);
        DebugDialog::debug("ListItem " + listItem);
        if (doc.setContent(listItem)) {
            listItems << listItem;
            break;
        }
        pos += listItem.count();
    }

    pos = 0 ;

    while (pos < data.count()) {
        int ix = data.indexOf(ListItemMatcher, pos);
        if (ix < 0) break;

        QString listItem2 = ListItemMatcher.cap(0);
        listItem2 += "/>" ;
        QString errorStr;
        int errorLine;
        int errorColumn;
        DebugDialog::debug("ListItem " + listItem2);
        if (doc.setContent(listItem2, &errorStr, &errorLine, &errorColumn)) {
            listItems << listItem2;
            break;
        }
        DebugDialog::debug("errorStr-----------------"+errorStr);
        DebugDialog::debug(QString("errorLine-----------------%1").arg(errorLine));
        DebugDialog::debug(QString("errorColumn-----------------%1").arg(errorColumn));

        pos += listItem2.count();
    }

    return listItems.join("");
}

void RecommendListWidget::getTutorialMessage(QNetworkReply * networkReply){

    QNetworkAccessManager * manager = networkReply->manager();
    int responseCode = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;
    if (responseCode == 200) {
        QString data(networkReply->readAll());
        data = "<thing>" + cleanData(data) + "</thing>";
        //DebugDialog::debug(data);

        if (doc.setContent(data, &errorStr, &errorLine, &errorColumn)) {
            readTutorial(doc);
        }

        DebugDialog::debug("errorStr-----------------"+errorStr);
        DebugDialog::debug(QString("errorLine-----------------%1").arg(errorLine));
        DebugDialog::debug(QString("errorColumn-----------------%1").arg(errorColumn));


    }
    manager->deleteLater();
    manager->clearAccessCache();
    networkReply->deleteLater();
    networkReply->close();
}

void RecommendListWidget::readTutorial(const QDomDocument & doc){

    QDomNodeList divList = doc.elementsByTagName("div");
    QDomElement div = divList.at(0).toElement();
    QString title = div.text();
    DebugDialog::debug("title-----------------" + title);
    Tutorial_title = title;

    QDomNodeList imgnodeList = doc.elementsByTagName("img");
    QDomElement img = imgnodeList.at(0).toElement();
    QString imgSrc = img.attribute("src");
    DebugDialog::debug("img-----------------" + imgSrc);
    Tutorial_img = imgSrc;


    /*
    QDomNodeList nodeList = doc.elementsByTagName("div");
        for (int i = 0; i < nodeList.count(); i++) {
            QDomElement element = nodeList.at(i).toElement();
            QDomElement child = element.firstChildElement();
            while (!child.isNull()) {
                if (child.tagName() == "img") {

                    QString imgSrc = child.attribute("src");
                    DebugDialog::debug("-----------------1");
                    DebugDialog::debug(imgSrc);
                    DebugDialog::debug("-----------------2");
                    DebugDialog::debug("readTutorial-----------------true head");
                    break;
                }


                child = child.nextSiblingElement();
                DebugDialog::debug("readTutorial-----------------");
            }

            DebugDialog::debug("nodeList-----------------count");
       }
    */
}

void RecommendListWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_startPoint = event->pos();
    }
    QListWidget::mousePressEvent(event);
}

void RecommendListWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        if ((event->pos() - m_startPoint).manhattanLength()
                >= QApplication::startDragDistance()) {
            execDrag();
        }
    }

    /*
    QListWidgetItem * item = itemAt(event->pos());
        if (item != NULL) {

        }
        else {
            DebugDialog::debug("mouseMoveEvent-----------------1");
        }
    */

    QListWidget::mouseMoveEvent(event);
}

void RecommendListWidget::execDrag(){
    QListWidgetItem * listitem = currentItem();
    if (listitem) {
           QMimeData *mimeData = new QMimeData;
           mimeData->setText(listitem->text());  // 設定所要攜帶的文字資訊
           //mimeData->setImageData(listitem->icon()); // 設定所要攜帶的影像資料

           QVariantList itemDataV = listitem->data(Qt::UserRole).toList();
           SuggestionType type = (SuggestionType)itemDataV[0].toInt();
           QByteArray itemData;
           QDataStream dataStream(&itemData, QIODevice::WriteOnly);

           if (type == SuggestionType::toModelSet) {
               //dataStream << itemDataV[1].value<QSharedPointer<ModelSet>>();
               //dataStream << itemDataV ;
               //dataStream << this->row(listitem);
               //mimeData->setData("type/toModelSet", itemData);
           } else {
               //dataStream << itemDataV[1].value<QSharedPointer<ModelSet>>() << itemDataV[2].value<QSharedPointer<SetConnection>>();
               dataStream << this->row(listitem);
               mimeData->setData("type/setToSet", itemData);
           }

           QDrag *drag = new QDrag(this);
           drag->setMimeData(mimeData);
           //drag->setPixmap(listitem->icon().pixmap(QSize(22, 22))); // 設定拖放時所顯示的圖示
           if (drag->exec(Qt::MoveAction) == Qt::MoveAction) {
               //delete listitem;
           }
    }
}


void RecommendListWidget::onItemClickedSlot(QListWidgetItem* listitem) {
    //onItemEvent(listitem, false);
    if (!mDoubleClicked) {
            QTimer::singleShot(300, this, SLOT(itemClickedTimeout()));
    }
    return;
}

void RecommendListWidget::itemClickedTimeout() {
    if (!mDoubleClicked) {
        // do something, listitem has been clicked once
        QListWidgetItem * listitem = currentItem();
        QVariantList itemDataV = listitem->data(Qt::UserRole).toList();
        SuggestionType type = (SuggestionType)itemDataV[0].toInt();
        if (type == SuggestionType::setToSet) {

            //QVariantList itemDataV = listitem->data(Qt::UserRole).toList();
            QVariantList tList = itemDataV[3].value<QVariantList>();
            m_tutorial->clear();

            for (int i = 0 ; i < tList.length() ; i++) {
                QString str = tList[i].value<QString>();

                QNetworkAccessManager * manager = new QNetworkAccessManager(this);
                QNetworkAccessManager * img_manager = new QNetworkAccessManager(this);
                QNetworkReply *netReply;
                QNetworkReply *netReply2;
                connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(getTutorialMessage(QNetworkReply *)));
                connect(img_manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(getTutorial_img(QNetworkReply *)));

                netReply = manager->get(QNetworkRequest(QUrl(str)));
                //manager->get(QNetworkRequest(QUrl(str)));
                //QEventLoop loop;
                connect(netReply, SIGNAL(finished()), &loop, SLOT(quit()));
                loop.exec();

                QUrl url("http://fritzing.org" + Tutorial_img);
                netReply2 = img_manager->get(QNetworkRequest(url));
                //img_manager->get(QNetworkRequest(url));
                //QEventLoop loop1;
                connect(netReply2, SIGNAL(finished()), &loop, SLOT(quit()));
                loop.exec();

                QLabel * label = new QLabel();
                label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
                label->setWordWrap(true);
                label->setAlignment(Qt::AlignCenter);
                label->setText(QString("<a href='%1'>%2</a>").arg(str).arg(Tutorial_title));
                label->setOpenExternalLinks(true);

                QLabel * img_label = new QLabel();
                img_label->setPixmap(pixmap.scaled(120,100,Qt::KeepAspectRatio));

                QHBoxLayout * hboxlayout = new QHBoxLayout;
                hboxlayout->addWidget(img_label);
                hboxlayout->addWidget(label);

                QWidget * window = new QWidget;
                window->setLayout(hboxlayout);
                window->setFixedHeight(100);
                window->setFixedWidth(250);

                QListWidgetItem* itemlist = new QListWidgetItem();
                itemlist->setSizeHint(QSize(250,100));
                //itemlist->setTextAlignment(Qt::AlignRight);
                //itemlist->setIcon(pixmap);
                m_tutorial->insertItem(i, itemlist);
                m_tutorial->setItemWidget(m_tutorial->item(i), window);

            }
        }


    } else mDoubleClicked = false;
}

void RecommendListWidget::onItemdDoubleClickedSlot(QListWidgetItem* listitem) {
    mDoubleClicked = true;
    onItemEvent(listitem, false);
    return;
}

void RecommendListWidget::onItemEvent(QListWidgetItem* listitem, bool hover) {
    //if (!hover) m_recommendlist->removeItemWidget(listitem);
    QVariantList itemDataV = listitem->data(Qt::UserRole).toList();
    SuggestionType type = (SuggestionType)itemDataV[0].toInt();
    if (type == SuggestionType::toModelSet) {
        m_sketchwidget->selectModelSet(itemDataV[1].value<QSharedPointer<ModelSet>>(), hover);
    } else {
        /*
        QVariantList tList = itemDataV[3].value<QVariantList>();
        m_tutorial->clear();

        for (int i = 0 ; i < tList.length() ; i++) {
            QString str = tList[i].value<QString>();


            QNetworkAccessManager * manager = new QNetworkAccessManager(this);
            QNetworkAccessManager * img_manager = new QNetworkAccessManager(this);
            QNetworkReply *netReply;
            connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(getTutorialMessage(QNetworkReply *)));
            connect(img_manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(getTutorial_img(QNetworkReply *)));
            //waitForMessage = true;
            netReply = manager->get(QNetworkRequest(QUrl(str)));
            //manager->get(QNetworkRequest(QUrl(str)));
            //QEventLoop loop;
            connect(netReply, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();
            DebugDialog::debug("onItemEvent-----------------1");
            QUrl url("http://fritzing.org" + Tutorial_img);
            //waitForImg = true;
            netReply = img_manager->get(QNetworkRequest(url));
            //img_manager->get(QNetworkRequest(url));
            //QEventLoop loop1;
            connect(netReply, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();
            DebugDialog::debug("onItemEvent-----------------2");

            QLabel * label = new QLabel();
            label->setAlignment(Qt::AlignCenter);
            label->setText(QString("<a href='%1'>%2</a>").arg(str).arg(Tutorial_title));
            DebugDialog::debug("onItemEvent-----------------3");
            //label->setText(Tutorial_title);
            DebugDialog::debug("onItemEvent-----------------4");
            label->setOpenExternalLinks(true);


            QListWidgetItem* itemlist = new QListWidgetItem();
            itemlist->setSizeHint(QSize(100,100));
            itemlist->setTextAlignment(Qt::AlignRight);
            itemlist->setIcon(pixmap);
            m_tutorial->insertItem(i, itemlist);
            m_tutorial->setItemWidget(m_tutorial->item(i), label);
            DebugDialog::debug("onItemEvent-----------------5");

        }
        */

        m_sketchwidget->selectSetToSet(itemDataV[1].value<QSharedPointer<ModelSet>>(), itemDataV[2].value<QSharedPointer<SetConnection>>(), false, hover) ;
    }

}

void RecommendListWidget::onItemEnteredSlot(QListWidgetItem* listitem){
    onItemEvent(listitem, true);
    return;
}

void RecommendListWidget::setTosetList(QList<QSharedPointer<ModelSet>> toModelsetList, QList<QSharedPointer<SetConnection>> setConnectionList, QList<QList<QString> *> tutorialLink){

    //m_recommendlist->clear();
    clear();
    for (int i = 0 ; i < toModelsetList.length() ; i++) {
        //QListWidgetItem* item = new QListWidgetItem(QString("%1 recommend").arg(i+1));
        QListWidgetItem* item = new QListWidgetItem();
        QVariantList itemData;
        itemData.append(QVariant(SuggestionType::setToSet));
        itemData.append(QVariant::fromValue(toModelsetList[i]));
        itemData.append(QVariant::fromValue(setConnectionList[i]));
        QVariantList variantList;
        //QVariantList img_List ;
        //QVariantList title_List;
        if (tutorialLink.length() > i) {
            foreach(QString str, *(tutorialLink[i])) {
                variantList.append(str);
                /*
                QNetworkAccessManager * manager = new QNetworkAccessManager(this);
                QNetworkAccessManager * img_manager = new QNetworkAccessManager(this);
                QNetworkReply *netReply;
                QNetworkReply *netReply2;
                connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(getTutorialMessage(QNetworkReply *)));
                connect(img_manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(getTutorial_img(QNetworkReply *)));
                netReply = manager->get(QNetworkRequest(QUrl(str)));
                connect(netReply, SIGNAL(finished()), &loop, SLOT(quit()));
                loop.exec();
                QUrl url("http://fritzing.org" + Tutorial_img);
                netReply2 = img_manager->get(QNetworkRequest(url));
                connect(netReply2, SIGNAL(finished()), &loop, SLOT(quit()));
                loop.exec();

                title_List.append(Tutorial_title);
                img_List.append(QVariant::fromValue(pixmap));
                */
            }
        }
        itemData.append(QVariant::fromValue(variantList));
        //itemData.append(QVariant::fromValue(title_List));
        //itemData.append(QVariant::fromValue(img_List));

        ModelPart * modelPart = m_referenceModel->retrieveModelPart(toModelsetList[i]->keyModuleID());
        loadImage(modelPart, item);
        item->setData(Qt::UserRole, itemData);
        item->setSizeHint(QSize(50,50));
        //m_recommendlist->insertItem(i, item) ;
        insertItem(i, item);

    }
}

void RecommendListWidget::setModelSetList(QList<QSharedPointer<ModelSet>> modelSetList){

    clear();

    for (int i = 0 ; i < modelSetList.length() ; i++) {
        //QListWidgetItem* item = new QListWidgetItem(QString("%1 recommend").arg(i+1));
        QListWidgetItem* item = new QListWidgetItem();
        QVariantList itemData;
        itemData.append(QVariant(SuggestionType::toModelSet));
        itemData.append(QVariant::fromValue(modelSetList[i]));
        ModelPart * modelPart = m_referenceModel->retrieveModelPart(modelSetList[i]->keyModuleID());
        loadImage(modelPart,item);
        item->setData(Qt::UserRole, itemData);
        item->setSizeHint(QSize(50,50));
        //m_recommendlist->insertItem(i ,item) ;
        insertItem(i, item);

    }
    m_sketchwidget->selectModelSet(modelSetList[0], true);

}

void RecommendListWidget::clearList() {
    //qDeleteAll(m_recommendlist->);
    //m_recommendlist->clear();
    clear();
}

void RecommendListWidget::loadImage(ModelPart * modelPart, QListWidgetItem * lwi)
{
    ItemBase * itemBase ;

    itemBase = PartFactory::createPart(modelPart, ViewLayer::NewTop, ViewLayer::IconView, ViewGeometry(), ItemBase::getNextID(), NULL, NULL, false);
    LayerAttributes layerAttributes ;
    itemBase->initLayerAttributes(layerAttributes, ViewLayer::IconView, ViewLayer::Icon, itemBase->viewLayerPlacement(), false, false);
    FSvgRenderer * renderer = itemBase->setUpImage(modelPart, layerAttributes);
    itemBase->setFilename(renderer->filename());
    itemBase->setSharedRendererEx(renderer);

    QSize size(HtmlInfoView::STANDARD_ICON_IMG_WIDTH, HtmlInfoView::STANDARD_ICON_IMG_HEIGHT);
    QPixmap * pixmap = FSvgRenderer::getPixmap(itemBase->renderer(), size);
    lwi->setIcon(QIcon(*pixmap));
    delete pixmap;
    lwi->setData(Qt::UserRole + 1, itemBase->renderer()->defaultSize());

}
