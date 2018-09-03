#include "recommendlistwidget.h"

#include <QSizeGrip>
#include <QStatusBar>
#include <QtDebug>
#include <QApplication>
#include <QListWidget>
#include <QLabel>

#include "../items/partfactory.h"
#include "../layerattributes.h"
#include "../fsvgrenderer.h"
#include "../infoview/htmlinfoview.h"

RecommendListWidget::RecommendListWidget(SketchWidget * sketchWidget, QPointer<ReferenceModel> referenceModel, QWidget *parent) {
	m_sketchwidget = sketchWidget;
    m_referenceModel = referenceModel;
	connect(this, SIGNAL(itemEntered(QListWidgetItem*)),
                       this, SLOT(onItemEnteredSlot(QListWidgetItem*)));

    connect(this, SIGNAL(itemClicked(QListWidgetItem*)),
                this, SLOT(onItemClickedSlot(QListWidgetItem*)));

    AutoCompleter * autocompleter = AutoCompleter::getAutoCompleter();

    connect(autocompleter, SIGNAL(addModelSetSignal(QList<QSharedPointer<ModelSet>>)),
                       this, SLOT(setModelSetList(QList<QSharedPointer<ModelSet>>)));
    connect(autocompleter, SIGNAL(addSetConnectionSignal(QList<QSharedPointer<ModelSet>>, QList<QSharedPointer<SetConnection>>)),
                       this, SLOT(setTosetList(QList<QSharedPointer<ModelSet>>, QList<QSharedPointer<SetConnection>>))) ;

    connect(autocompleter, SIGNAL(clearRecommendListSignal()), this, SLOT(clearList()));
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
        m_sketchwidget->selectSetToSet(itemDataV[1].value<QSharedPointer<ModelSet>>(), itemDataV[2].value<QSharedPointer<SetConnection>>(), false, hover) ;
    }

}

void RecommendListWidget::onItemEnteredSlot(QListWidgetItem* listitem){
    onItemEvent(listitem, true);
    return;
}

void RecommendListWidget::setTosetList(QList<QSharedPointer<ModelSet>> toModelsetList, QList<QSharedPointer<SetConnection>> setConnectionList){

    //m_recommendlist->clear();
    clear();
    for (int i = 0 ; i < toModelsetList.length() ; i++) {
        //QListWidgetItem* item = new QListWidgetItem(QString("%1 recommend").arg(i+1));
        QListWidgetItem* item = new QListWidgetItem();
        QVariantList itemData;
        itemData.append(QVariant(SuggestionType::setToSet));
        itemData.append(QVariant::fromValue(toModelsetList[i]));
        itemData.append(QVariant::fromValue(setConnectionList[i]));

        ModelPart * modelPart = m_referenceModel->retrieveModelPart(toModelsetList[i]->keyModuleID());
        loadImage(modelPart, item);
        item->setData(Qt::UserRole, itemData);
        item->setSizeHint(QSize(50,60));
        //m_recommendlist->insertItem(i, item) ;
        insertItem(i, item);

        QLabel * label = new QLabel();
        label->setAlignment(Qt::AlignCenter);
        label->setText("<a href=\"https://github.com/lojoyu/fritzing-app\">github");
        label->setOpenExternalLinks(true);
        //m_recommendlist->setItemWidget(m_recommendlist->item(i),label);
        setItemWidget(this->item(i), label);
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
        item->setSizeHint(QSize(50,60));
        //m_recommendlist->insertItem(i ,item) ;
        insertItem(i, item);

        QLabel * label = new QLabel();
        label->setAlignment(Qt::AlignCenter);
        label->setText("<a href=\"https://github.com/lojoyu/fritzing-app\">github");
        label->setOpenExternalLinks(true);
        //m_recommendlist->setItemWidget(m_recommendlist->item(i),label);
        setItemWidget(this->item(i), label);
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
