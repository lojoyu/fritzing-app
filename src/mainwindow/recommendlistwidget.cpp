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
#include "../debugdialog.h"
#include "../items/moduleidnames.h"

static QString BlackStar("★");
static QString WhiteStar("☆");
//٭
//QChar(0xe2 0x98 0x85)

RecommendListWidget::RecommendListWidget(SketchWidget * sketchWidget, QPointer<ReferenceModel> referenceModel, QListWidget * tutorialList, QWidget *parent) {
	m_sketchwidget = sketchWidget;
    m_referenceModel = referenceModel;
    m_tutorial = tutorialList;
	connect(this, SIGNAL(itemEntered(QListWidgetItem*)),
                       this, SLOT(onItemEnteredSlot(QListWidgetItem*)));

    connect(this, SIGNAL(itemClicked(QListWidgetItem*)),
                this, SLOT(onItemClickedSlot(QListWidgetItem*)));

    AutoCompleter * autocompleter = AutoCompleter::getAutoCompleter();

    connect(autocompleter, SIGNAL(addModelSetSignal(QList<QSharedPointer<ModelSet>>, QList<double>)),
                       this, SLOT(setModelSetList(QList<QSharedPointer<ModelSet>>, QList<double>)));
    connect(autocompleter, SIGNAL(addSetConnectionSignal(QList<QSharedPointer<ModelSet>>, QList<QSharedPointer<SetConnection>>, QList<QList<QString> *>, QList<double>, bool)),
                       this, SLOT(setTosetList(QList<QSharedPointer<ModelSet>>, QList<QSharedPointer<SetConnection>>, QList<QList<QString> *>, QList<double>, bool))) ;

    connect(autocompleter, SIGNAL(clearRecommendListSignal()), this, SLOT(clearList()));

    m_modelSetImgPath = "/Users/lojoyu/Desktop/fritzing_a1_icon.png";
   // m_connectionImgPath
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
        m_sketchwidget->selectModelSet(itemDataV[1].value<QSharedPointer<ModelSet>>(), hover, this->count() > 1 ? false : true);
    } else {

        bool connection = type == SuggestionType::setToSet ? false : true;

        m_sketchwidget->selectSetToSet(itemDataV[1].value<QSharedPointer<ModelSet>>(), itemDataV[2].value<QSharedPointer<SetConnection>>(), connection, hover) ;

        QVariantList tList = itemDataV[3].value<QVariantList>();
        m_tutorial->clear();
        for (int i = 0 ; i < tList.length() ; i++) {
            QString str = tList[i].value<QString>();
            QLabel * label = new QLabel();
            label->setAlignment(Qt::AlignLeft);
            // label->setText(QString("<a href='%1'>tutorial %2</a>").arg(str).arg(i));
            QString title = str.split("/").last().replace("-", " ");
            label->setText(QString("<a href='%1'>%2</a>").arg(str).arg(title));
            label->setOpenExternalLinks(true);
            //layout->addWidget(label);
            QListWidgetItem* item = new QListWidgetItem();
            m_tutorial->insertItem(i, item);
            m_tutorial->setItemWidget(item, label);

        }

    }

}

void RecommendListWidget::onItemEnteredSlot(QListWidgetItem* listitem){
    onItemEvent(listitem, true);
    return;
}

void RecommendListWidget::setTosetList(QList<QSharedPointer<ModelSet>> toModelsetList, QList<QSharedPointer<SetConnection>> setConnectionList,
                                       QList<QList<QString> *> tutorialLink, QList<double> percentageList, bool connection){

    //m_recommendlist->clear();
    clear();
    m_tutorial->clear();
    for (int i = 0 ; i < toModelsetList.length() ; i++) {
        //QListWidgetItem* item = new QListWidgetItem(QString("%1 recommend").arg(i+1));
        QListWidgetItem* item = new QListWidgetItem();
        QVariantList itemData;
        if (!connection) itemData.append(QVariant(SuggestionType::setToSet));
        else itemData.append(QVariant(SuggestionType::connection));
        itemData.append(QVariant::fromValue(toModelsetList[i]));
        if (setConnectionList.length() > i) itemData.append(QVariant::fromValue(setConnectionList[i]));
        else itemData.append(QVariant::fromValue(QSharedPointer<SetConnection>(NULL)));
        QVariantList variantList;
        if (tutorialLink.length() > i) {
            foreach(QString str, *(tutorialLink[i])) {
                variantList.append(str);
            }
        }
        itemData.append(QVariant::fromValue(variantList));
        QString mid;
        if (connection) mid = ModuleIDNames::WireModuleIDName;
        else mid = toModelsetList[i]->keyModuleID();

        ModelPart * modelPart = m_referenceModel->retrieveModelPart(mid);
        loadImage(modelPart, item);
        item->setData(Qt::UserRole, itemData);
        item->setSizeHint(QSize(50,60));
        //m_recommendlist->insertItem(i, item) ;
        insertItem(i, item);
        double percentage = percentageList[i];
        QString text = QString("%1").arg(BlackStar);
        if (percentage > 0.3) text += BlackStar;
        if (percentage > 0.2) text += BlackStar;
        if (percentage > 0.1) text += BlackStar;
        if (percentage > 0.05) text += BlackStar;

        item->setText(QString("%1").arg(text));

        QFont f = item->font();
        f.setPointSize(8);
        item->setFont(f);
        item->setTextColor(QColor(252, 193, 0));
    }
    if (connection) {
        m_sketchwidget->selectSetToSet(toModelsetList[0], setConnectionList[0], true, true);
    }
}

void RecommendListWidget::setModelSetList(QList<QSharedPointer<ModelSet>> modelSetList, QList<double> percentageList){

    clear();
    m_tutorial->clear();

    for (int i = 0 ; i < modelSetList.length() ; i++) {
        //QListWidgetItem* item = new QListWidgetItem(QString("%1 recommend").arg(i+1));
        QListWidgetItem* item = new QListWidgetItem();
        QVariantList itemData;
        itemData.append(QVariant(SuggestionType::toModelSet));
        itemData.append(QVariant::fromValue(modelSetList[i]));
        //ModelPart * modelPart = m_referenceModel->retrieveModelPart(modelSetList[i]->keyModuleID());
        //loadImage(modelPart,item);
        loadImage(m_modelSetImgPath, item);
        item->setData(Qt::UserRole, itemData);
        item->setSizeHint(QSize(50,60));
        double percentage = percentageList[i];
        QString text = QString("%1").arg(BlackStar);
        if (percentage > 0.4) text += BlackStar;
        if (percentage > 0.3) text += BlackStar;
        if (percentage > 0.2) text += BlackStar;
        if (percentage > 0.1) text += BlackStar;

        item->setText(QString("%1").arg(text));
        //m_recommendlist->insertItem(i ,item) ;
        insertItem(i, item);
        QFont f = item->font();
        f.setPointSize(8);
        item->setFont(f);
        item->setTextColor(QColor(252, 193, 0));

//        QLabel * label = new QLabel();
//        label->setAlignment(Qt::AlignLeft);
//        label->setText(QString("%1").arg(percentage));
//        setItemWidget(this->item(i), label);


    }
    bool next = this->count() > 1 ? false : true;
    m_sketchwidget->selectModelSet(modelSetList[0], true, next);

}

void RecommendListWidget::clearList() {
    //qDeleteAll(m_recommendlist->);
    //m_recommendlist->clear();
    clear();
    m_tutorial->clear();
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

void RecommendListWidget::loadImage(QString filePath, QListWidgetItem * lwi)
{
    QSize size(HtmlInfoView::STANDARD_ICON_IMG_WIDTH, HtmlInfoView::STANDARD_ICON_IMG_HEIGHT);
    QPixmap pixmap = QPixmap(filePath);
    lwi->setIcon(QIcon(pixmap));
    //lwi->setData(Qt::UserRole + 1, itemBase->renderer()->defaultSize());

}
