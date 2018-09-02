#include <QSizeGrip>
#include <QStatusBar>
#include <QtDebug>
#include <QApplication>
#include <QListWidget>

#include "mainwindow.h"
#include "../sketch/sketchwidget.h"
#include "../autocomplete/autocompleter.h"
#include "../debugdialog.h"
#include "../items/partfactory.h"
#include "../layerattributes.h"
#include "../infoview/htmlinfoview.h"
#include "../fsvgrenderer.h"

void MainWindow::initRecommendList(SketchWidget * sketchWidget){

    m_sketchwidget = sketchWidget ;
    m_recommendlist = new QListWidget ;
    m_recommendlist->setMouseTracking(true) ;
    m_recommendlist->setViewMode(QListWidget::IconMode);
    makeDock(tr("Recommand List"), m_recommendlist, 30, 30);

    connect(m_recommendlist, SIGNAL(itemEntered(QListWidgetItem*)),
                       this, SLOT(onItemEnteredSlot(QListWidgetItem*)));

    connect(m_recommendlist, SIGNAL(itemClicked(QListWidgetItem*)),
                this, SLOT(onItemClickedSlot(QListWidgetItem*)));

    AutoCompleter * autocompleter = AutoCompleter::getAutoCompleter();

    connect(autocompleter, SIGNAL(addModelSetSignal(QList<QSharedPointer<ModelSet>>)),
                       this, SLOT(setModelSetList(QList<QSharedPointer<ModelSet>>)));
    connect(autocompleter, SIGNAL(addSetConnectionSignal(QList<QSharedPointer<ModelSet>>, QList<QSharedPointer<SetConnection>>)),
                       this, SLOT(setTosetList(QList<QSharedPointer<ModelSet>>, QList<QSharedPointer<SetConnection>>))) ;

    connect(autocompleter, SIGNAL(clearRecommendListSignal()), this, SLOT(clearList()));
}

//void MainWindow::CurrentRowItem(int i){

//    if ( b_setmodelSet ) {
//        if (m_modelSetList_1.length() > 0) {
//            QSharedPointer<ModelSet> modelSet = m_modelSetList_1[i];
//            m_modelSetList_1.clear();
//            sketchwidget_1->addModelSet(modelSet,false) ;
//        }
//    }

//    if ( b_setToset ) {
//        QSharedPointer<ModelSet> modelSet = m_modelSetList_1[i];
//        QSharedPointer<SetConnection> setConnection = m_setConnectionList_1[i];
//        m_modelSetList_1.clear();
//        Recommendlist_1->clear();
//        sketchwidget_1->addSetToSet(m_toModelsetList_1[i], m_setConnectionList_1[i],false) ;
//    }

//}

void MainWindow::onItemClickedSlot(QListWidgetItem* listitem) {
    onItemEvent(listitem, false);
    return;
}

void MainWindow::onItemEvent(QListWidgetItem* listitem, bool hover) {
    //if (!hover) m_recommendlist->removeItemWidget(listitem);
    QVariantList itemDataV = listitem->data(Qt::UserRole).toList();
    SuggestionType type = (SuggestionType)itemDataV[0].toInt();
    if (type == SuggestionType::toModelSet) {
        m_sketchwidget->selectModelSet(itemDataV[1].value<QSharedPointer<ModelSet>>(), hover);
    } else {
        m_sketchwidget->selectSetToSet(itemDataV[1].value<QSharedPointer<ModelSet>>(), itemDataV[2].value<QSharedPointer<SetConnection>>(),  hover) ;
    }

}

void MainWindow::onItemEnteredSlot(QListWidgetItem* listitem){
    onItemEvent(listitem, true);
    return;
}

void MainWindow::setTosetList(QList<QSharedPointer<ModelSet>> toModelsetList, QList<QSharedPointer<SetConnection>> setConnectionList){

    m_recommendlist->clear();
    for (int i = 0 ; i < toModelsetList.length() ; i++) {
        //QListWidgetItem* item = new QListWidgetItem(QString("%1 recommend").arg(i+1));
        QListWidgetItem* item = new QListWidgetItem();
        QVariantList itemData;
        itemData.append(QVariant(SuggestionType::setToSet));
        itemData.append(QVariant::fromValue(toModelsetList[i]));
        itemData.append(QVariant::fromValue(setConnectionList[i]));
        ModelPart * modelPart = m_referenceModel->retrieveModelPart(toModelsetList[i]->keyModuleID());
        loadImage(modelPart,item);
        item->setData(Qt::UserRole, itemData);
        item->setSizeHint(QSize(50,60));
        m_recommendlist->insertItem(i ,item) ;

        QLabel * label = new QLabel();
        label->setAlignment(Qt::AlignCenter);
        label->setText("<a href=\"https://github.com/lojoyu/fritzing-app\">github");
        label->setOpenExternalLinks(true);
        m_recommendlist->setItemWidget(m_recommendlist->item(i),label);
    }
}

void MainWindow::setModelSetList(QList<QSharedPointer<ModelSet>> modelSetList){

    m_recommendlist->clear();

    for (int i = 0 ; i < modelSetList.length() ; i++) {
        //QListWidgetItem* item = new QListWidgetItem(QString("%1 recommend").arg(i+1));
        QListWidgetItem* item = new QListWidgetItem();
        QVariantList itemData;
        itemData.append(QVariant(SuggestionType::toModelSet));
        itemData.append(QVariant::fromValue(modelSetList[i]));
        ModelPart * modelPart = m_referenceModel->retrieveModelPart(modelSetList[i]->keyModuleID());
        loadImage(modelPart,item);
        item->setData(Qt::UserRole, itemData);
        item->setSizeHint(QSize(50,60));
        m_recommendlist->insertItem(i ,item) ;


        QLabel * label = new QLabel();
        label->setAlignment(Qt::AlignCenter);
        label->setText("<a href=\"https://github.com/lojoyu/fritzing-app\">github");
        label->setOpenExternalLinks(true);
        m_recommendlist->setItemWidget(m_recommendlist->item(i),label);


    }
    m_sketchwidget->selectModelSet(modelSetList[0], true);

}

void MainWindow::clearList() {
    //qDeleteAll(m_recommendlist->);
    m_recommendlist->clear();
}

void MainWindow::loadImage(ModelPart * modelPart, QListWidgetItem * lwi)
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

//void MainWindow::addRecommendList(QList<QSharedPointer<ModelSet>> toModelsetList, QList<QSharedPointer<SetConnection>> setConnectionList){

//    m_toModelsetList = toModelsetList ;
//    m_setConnectionList = setConnectionList ;

//    Recommendlist.clear() ;

//    for (int i = 0 ; i < toModelsetList.length(); i++) {
//        Recommendlist->insertItem(i,new QListWidgetItem(QString("%1 recommend").arg(i))) ;
//    }

//}

//void MainWindow::addmodelSetList(QList<QSharedPointer<ModelSet>> modelSetList){
//    m_modelSetList = modelSetList ;


//}
