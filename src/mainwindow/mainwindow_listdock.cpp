#include <QSizeGrip>
#include <QStatusBar>
#include <QtDebug>
#include <QApplication>
#include <QListWidget>

#include "mainwindow.h"
#include "../sketch/sketchwidget.h"
#include "../autocomplete/autocompleter.h"
#include "../debugdialog.h"


void MainWindow::initRecommendList(SketchWidget * sketchWidget){

    m_sketchwidget = sketchWidget ;
    m_recommendlist = new QListWidget ;
    m_recommendlist->setMouseTracking(true) ;
    makeDock(tr("Recommand List"), m_recommendlist, 30, 30);

    connect(m_recommendlist, SIGNAL(itemEntered(QListWidgetItem*)),
                       this, SLOT(onItemEnteredSlot(QListWidgetItem*)));

    connect(m_recommendlist, SIGNAL(itemClicked(QListWidgetItem*)),
                this, SLOT(onItemClickedSlot(QListWidgetItem*)));

    AutoCompleter * autocompleter = AutoCompleter::getAutoCompleter();

    connect(autocompleter, SIGNAL(addModelSetSignal(QList<QSharedPointer<ModelSet>>)),
                       this, SLOT(setmodelSetList(QList<QSharedPointer<ModelSet>>)));
    connect(autocompleter, SIGNAL(addSetConnectionSignal(QList<QSharedPointer<ModelSet>>, QList<QSharedPointer<SetConnection>>)),
                       this, SLOT(setTosetList(QList<QSharedPointer<ModelSet>>, QList<QSharedPointer<SetConnection>>))) ;

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

    QVariantList itemDataV = listitem->data(Qt::UserRole).toList();
    SuggestionType type = (SuggestionType)itemDataV[0].toInt();
    if (type == SuggestionType::toModelSet) {
        m_sketchwidget->addModelSet(itemDataV[1].value<QSharedPointer<ModelSet>>(), hover) ;
    } else {
        m_sketchwidget->addSetToSet(itemDataV[1].value<QSharedPointer<ModelSet>>(), itemDataV[2].value<QSharedPointer<SetConnection>>(),  hover) ;
    }
}

void MainWindow::onItemEnteredSlot(QListWidgetItem* listitem){
    onItemEvent(listitem, true);
    return;
}

void MainWindow::setTosetList(QList<QSharedPointer<ModelSet>> toModelsetList, QList<QSharedPointer<SetConnection>> setConnectionList){

    m_recommendlist->clear();
    for (int i = 0 ; i < toModelsetList.length() ; i++) {
        QListWidgetItem* item = new QListWidgetItem(QString("%1 recommend").arg(i+1));
        QVariantList itemData;
        itemData.append(QVariant(SuggestionType::setToSet));
        itemData.append(QVariant::fromValue(toModelsetList[i]));
        itemData.append(QVariant::fromValue(setConnectionList[i]));
        item->setData(Qt::UserRole, itemData);
        m_recommendlist->insertItem(i ,item) ;
    }
}

void MainWindow::setmodelSetList(QList<QSharedPointer<ModelSet>> modelSetList){


    m_recommendlist->clear() ;

    for (int i = 0 ; i < modelSetList.length() ; i++) {
        QListWidgetItem* item = new QListWidgetItem(QString("%1 recommend").arg(i+1));
        QVariantList itemData;
        itemData.append(QVariant(SuggestionType::toModelSet));
        itemData.append(QVariant::fromValue(modelSetList[i]));
        item->setData(Qt::UserRole, itemData);
        m_recommendlist->insertItem(i ,item) ;
    }

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
