#include <QSizeGrip>
#include <QStatusBar>
#include <QtDebug>
#include <QApplication>
#include <QListWidget>

#include "mainwindow.h"
#include "../fritzing-app/src/sketch/sketchwidget.h"
#include "../fritzing-app/src/autocomplete/autocompleter.h"
#include "../debugdialog.h"

QList<ModelSet *> m_modelSetList_1 ;
QList<ModelSet *> m_toModelsetList_1 ;
QList<SetConnection *> m_setConnectionList_1 ;
QListWidget * Recommendlist_1 ;
SketchWidget *sketchwidget_1 ;
bool b_setmodelSet ;
bool b_setToset ;

void MainWindow::initRecommendList(SketchWidget * sketchWidget){

    sketchwidget_1 = sketchWidget ;
    Recommendlist_1 = new QListWidget ;
    Recommendlist_1->setMouseTracking(true) ;
    makeDock(tr("Recommand List"), Recommendlist_1, 30, 30);

    connect(Recommendlist_1, SIGNAL(currentRowChanged(int)),
                       this, SLOT(CurrentRowItem(int))) ;

    connect(Recommendlist_1, SIGNAL(itemEntered(QListWidgetItem*)),
                       this, SLOT(slotOnItemEntered(QListWidgetItem*))) ;

    /*
    connect(AutoCompleter::getAutoCompleter(), SIGNAL(SetSelf_signal(QList<ModelSet *>)),
                       this, SLOT(addmodelSetList(QList<ModelSet *>))) ;

    connect(AutoCompleter::getAutoCompleter(), SIGNAL(NextSelf_signal(QList<ModelSet *>, QList<SetConnection *>)),
                       this, SLOT(addRecommendList(QList<ModelSet *>, QList<SetConnection *>))) ;

    */

}

void MainWindow::CurrentRowItem(int i){

    if ( b_setmodelSet ) {
        if (m_modelSetList_1.length() > 0) {
            sketchwidget_1->addModelSet(m_modelSetList_1[i],false) ;
        }
    }

    if ( b_setToset ) {
        sketchwidget_1->addSetToSet(m_toModelsetList_1[i], m_setConnectionList_1[i],false) ;
    }

}

void MainWindow::slotOnItemEntered(QListWidgetItem* listitem){
    
    if ( b_setmodelSet ) {
        //DebugDialog::debug(QString("%1 : length-------------------").arg(m_modelSetList_1.length()));
        //DebugDialog::debug(QString("%1 : row-------------------").arg(listitem->listWidget()->row(listitem)));
        if (m_modelSetList_1.length() > 0) {
            sketchwidget_1->addModelSet(m_modelSetList_1[listitem->listWidget()->row(listitem)],true) ;
        }

    }

    if ( b_setToset ) {
        sketchwidget_1->addSetToSet(m_toModelsetList_1[listitem->listWidget()->row(listitem)], m_setConnectionList_1[listitem->listWidget()->row(listitem)],true) ;
    }

}

void MainWindow::setTosetList(QList<ModelSet *> toModelsetList, QList<SetConnection *> setConnectionList){
    b_setmodelSet = false ;
    b_setToset = true ;
    
    m_toModelsetList_1 = toModelsetList ;
    m_setConnectionList_1 = setConnectionList ;

    Recommendlist_1->clear();

    for (int i = 0 ; i < toModelsetList.length() ; i++) {
        Recommendlist_1->insertItem(i,new QListWidgetItem(QString("%1 recommend").arg(i+1))) ;
    }
}

void MainWindow::setmodelSetList(QList<ModelSet *> modelSetList){
    b_setmodelSet = true ;
    b_setToset = false ;

    m_modelSetList_1 = modelSetList ;
    Recommendlist_1->clear() ;

    for (int i = 0 ; i < modelSetList.length() ; i++) {
        Recommendlist_1->insertItem(i,new QListWidgetItem(QString("%1 recommend").arg(i+1))) ;
    }
    /*
    for (int i = 0 ; i < modelSetList.length() ; i++ ){
        DebugDialog::debug("setmodelSetList----------------------------1.5");
        m_modelSetList.append(modelSetList[i]);
    }
    */
}

void MainWindow::addRecommendList(QList<ModelSet *> toModelsetList, QList<SetConnection *> setConnectionList){

    m_toModelsetList = toModelsetList ;
    m_setConnectionList = setConnectionList ;

    Recommendlist.clear() ;

    for (int i = 0 ; i < toModelsetList.length(); i++) {
        Recommendlist->insertItem(i,new QListWidgetItem(QString("%1 recommend").arg(i))) ;
    }

}

void MainWindow::addmodelSetList(QList<ModelSet *> modelSetList){
    m_modelSetList = modelSetList ;


}
