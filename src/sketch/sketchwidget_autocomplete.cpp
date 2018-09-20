#include <QtCore>
#include <QGraphicsScene>
#include <QGraphicsView>

#include "sketchwidget.h"
#include "../autocomplete/autocompleter.h"
#include "../debugdialog.h"
#include "../connectors/connectoritem.h"
#include "../waitpushundostack.h"
#include "../items/wire.h"
#include "../utils/bezier.h"

typedef QPair<long, long> LongLongPair;
typedef QPair<QString, QString> StringPair;
typedef QPair<ModelSet::Terminal, QString> TerminalStringPair;
typedef QPair<ItemBase*, QString> ItemBaseStringPair;

bool SketchWidget::isLeft(QPointF p1, QPointF p2, QPointF pin){
    return ((p2.x() - p1.x())*(pin.y() - p1.y()) - (p2.y() - p1.y())*(pin.x() - p1.x())) > 0;
}

void SketchWidget::setAutoComplete(bool autoComplete) {
	m_autoComplete = autoComplete;
    autocompleteInit();
}

void SketchWidget::autocompleteInit() {
    if (!m_autoComplete) return;
    if (m_addedDefaultPart.isNull()) return;
    QString title = m_addedDefaultPart->title();
    QString moduleID = m_addedDefaultPart->moduleID();
    QString label = m_addedDefaultPart->label();
    m_breadBoardModelSet = QSharedPointer<ModelSet>(new ModelSet(-1, moduleID));
    m_breadBoardModelSet->setKeyLabel(label);
    m_breadBoardModelSet->setKeyItem(m_addedDefaultPart);
    foreach(ConnectorItem * connectorItem, m_addedDefaultPart->cachedConnectorItems()) {
        DebugDialog::debug(QString("---%1").arg(connectorItem->connectorSharedID()));
        ModelSet::Terminal t1 = ModelSet::Terminal(moduleID, title, label, connectorItem->connectorSharedID(), "", "");
        ModelSet::Terminal t2 = ModelSet::Terminal("NULL", "NULL", "NULL", "NULL", "", "");
        m_breadBoardModelSet->appendConnection(t1, t2);
        m_breadBoardModelSet->insertTerminalnameHash(connectorItem->connectorSharedID(), t1);
    }
    m_breadBoardGnd = false;
    //testWire();
}

void SketchWidget::testWire() {

//    ItemBase * wireItem = addSetWire(m_addedDefaultPart, "pin61Z", m_addedDefaultPart, "pin61X", false);
//    Wire* w = qobject_cast<Wire*>(wireItem);
//    if (w) w->setColorString("black", 1, true);
//    return;
    ModelPart * wirePart = m_referenceModel->retrieveModelPart(QString("WireModuleID"));
    long wireID = ItemBase::getNextID();
    ViewGeometry viewGeometryWire;
    QLineF line;
    line.setLine(0, 0, 100, 100);

    viewGeometryWire.setLoc(QPointF(30, 50));
    viewGeometryWire.setLine(line);
    ItemBase * wireItem = addItemAuxTemp(wirePart, defaultViewLayerPlacement(wirePart), viewGeometryWire, wireID, true, m_viewID, true);

    Wire * wire = qobject_cast<Wire *>(wireItem);
    if (wire) {
        wire->setColorString("red", 1, true);
        //wire->setColor(QColor(255, 0, 0), 1);
        updateInfoView();
    }
    //wire->setColor(QColor(255, 0, 0), 1);

    //wire->connector0()->connectTo(m_addedDefaultPart->findConnectorItemWithSharedID("pin3W"));
    changeConnection(m_addedDefaultPart->id(), "pin3W", wireID, "connector0", defaultViewLayerPlacement(wirePart), true, false, false);


    wire->saveGeometry();
    ViewGeometry vg = wire->getViewGeometry();

    QPointF oldPos = wire->pos();
    QLineF oldLine = wire->line();
    QPointF newPos = QPointF(oldLine.p1().x(), oldLine.p2().y())+oldPos;

    Bezier left(oldPos, newPos), right(oldPos, newPos);
    //bool curved = wire->initNewBendpoint(newPos, left, right);
    bool curved = true;
    QLineF newLine(oldLine.p1(), newPos - oldPos);
    wire->setLine(newLine);
    if (curved) {

        QPointF leftB1(newPos.x() - 50, newPos.x() - 50);
        QPointF leftB2(newPos.x() + 50, newPos.x() + 50);
        left = Bezier(QPointF(-20, 0), QPointF(-20, 100));
        right = Bezier(QPointF(50, 50), QPointF(50, 50));


    }
    if (curved) wire->changeCurve(&left);
    vg.setLoc(newPos);

    QLineF newLine2(QPointF(0,0), oldLine.p2() + oldPos - newPos);
    vg.setLine(newLine2);

    ConnectorItem * oldConnector1 = wire->connector1();
    Wire * newWire = this->createTempWireForDragging(wire, wire->modelPart(), oldConnector1, vg, wire->viewLayerPlacement());
    if (curved) {
        right.translateToZero();
        newWire->changeCurve(&right);
    }
    ConnectorItem * newConnector1 = newWire->connector1();
    foreach (ConnectorItem * toConnectorItem, oldConnector1->connectedToItems()) {
        oldConnector1->tempRemove(toConnectorItem, false);
        toConnectorItem->tempRemove(oldConnector1, false);
        newConnector1->tempConnectTo(toConnectorItem, false);
        toConnectorItem->tempConnectTo(newConnector1, false);
    }
    oldConnector1->tempConnectTo(newWire->connector0(), false);
    newWire->connector0()->tempConnectTo(oldConnector1, false);

}

Wire * SketchWidget::squareWire(ItemBase* wireItem, QPointF newPos) {
    Wire * wire = qobject_cast<Wire *>(wireItem);
    if (!wire) return NULL;

    wire->saveGeometry();
    ViewGeometry vg = wire->getViewGeometry();

    QPointF oldPos = wire->pos();
    QLineF oldLine = wire->line();
    //QPointF newPos = QPointF(oldLine.p1().x(), oldLine.p2().y())+oldPos;

    Bezier left, right;
    bool curved = wire->initNewBendpoint(newPos, left, right);
    QLineF newLine(oldLine.p1(), newPos - oldPos);
    wire->setLine(newLine);

    //if (curved) wire->changeCurve(&left);
    vg.setLoc(newPos);

    QLineF newLine2(QPointF(0,0), oldLine.p2() + oldPos - newPos);
    vg.setLine(newLine2);

    ConnectorItem * oldConnector1 = wire->connector1();
    //Wire * newWire = createTempWireForDragging(wire, wire->modelPart(), oldConnector1, vg, wire->viewLayerPlacement());
    //bool temp = wireItem->opacity() == 1 ? false : true;
    bool temp = false;
    ItemBase * newWireItem = addItemAuxTemp(wire->modelPart(), wire->viewLayerPlacement(), vg, ItemBase::getNextID(), true, m_viewID, temp);
    Wire * newWire = qobject_cast<Wire*>(newWireItem);

    ConnectorItem * newConnector1 = newWire->connector1();
    foreach (ConnectorItem * toConnectorItem, oldConnector1->connectedToItems()) {
        oldConnector1->tempRemove(toConnectorItem, false);
        toConnectorItem->tempRemove(oldConnector1, false);
        newConnector1->tempConnectTo(toConnectorItem, true);
        toConnectorItem->tempConnectTo(newConnector1, true);
        //newConnector1->connectTo(toConnectorItem);
        //toConnectorItem->connectTo(newConnector1);
    }
    oldConnector1->tempConnectTo(newWire->connector0(), true);
    newWire->connector0()->tempConnectTo(oldConnector1, true);
    //oldConnector1->connectTo(newWire->connector0());
    //newWire->connector0()->connectTo(oldConnector1);
    //changeConnection(wire->id(), "connector1", newWire->id(), "connector0", wire->viewLayerPlacement(), true, false, true);
    changeConnection(newWire->id(), "connector0", wire->id(), "connector1", wire->viewLayerPlacement(), true, false, true);

    //newWire->initDragEnd(newWire->connector0(), newPos);

    this->update();

    return newWire;
}

double SketchWidget::fRand(double fMin, double fMax){
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

QList<ItemBase *> SketchWidget::arrangeWire(ItemBase* wireItem, ItemBase * fromItem, const QString & fromConnectorID,
                              ItemBase * toItem, const QString & toConnectorID) {


    ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, fromItem->viewLayerPlacement());
    ConnectorItem * toConnectorItem = findConnectorItem(toItem, toConnectorID, toItem->viewLayerPlacement());
    if (!fromConnectorItem || !toConnectorItem ) return QList<ItemBase *>();

    QString from = fromItem->title();
    QString to = toItem->title();
    QPointF fromConnectorPos = fromConnectorItem->sceneAdjustedTerminalPoint(NULL);
    QPointF toConnectorPos = toConnectorItem->sceneAdjustedTerminalPoint(NULL);
    QPointF moveVector = toConnectorPos - fromConnectorPos;

    ConnectorArrange fromConnectorArrange = getConnectorArrange(fromItem, fromConnectorPos);
    ConnectorArrange toConnectorArrange = getConnectorArrange(toItem, toConnectorPos);

    QList<QPointF> moveList({QPointF(0, -1), QPointF(0, 1), QPointF(-1, 0), QPointF(1, 0)});
    QPointF fromMove = moveList[fromConnectorArrange];
    QPointF fromAxis(qAbs(fromMove.x()), qAbs(fromMove.y()));
    QPointF toMove = moveList[toConnectorArrange];
    QPointF toAxis(qAbs(toMove.x()), qAbs(toMove.y()));

    QPoint verMove(qAbs(fromMove.y()), qAbs(fromMove.x()));
    double sign = QPointF::dotProduct(moveVector, verMove) >= 0 ? 1 : -1;

    QPointF fromAddTo = fromMove+toMove;
    bool fromSide = QPointF::dotProduct(moveVector, fromMove) >= 0 ? true : false;
    bool toSide = QPointF::dotProduct(-moveVector, toMove) >= 0 ? true : false;
    QList<QPointF> turnPos;

    QPointF selfR(20, 20);
    QPointF lowestR(5, 5);
//    selfR += m_prevRandom;
//    lowestR += m_prevRandom;
    double dist = qAbs(QPointF::dotProduct(moveVector, fromAxis));
    double verDist = qAbs(QPointF::dotProduct(moveVector, verMove));

    if (fromAddTo.x() == 0 || fromAddTo.y() == 0) { // same axis
        if (fromSide && toSide) { //face to face
            double randomBtw = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                    dist - QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = randomBtw * fromAxis;
            QPointF first = fromConnectorPos + randomBtw * fromMove;
            turnPos.append(first);
            if (fromAddTo.x() == 0) turnPos.append(QPointF(toConnectorPos.x(), first.y()));
            else turnPos.append(QPointF(first.x(), toConnectorPos.y()));

        } else if (fromSide && !toSide) { // same direction + from true
            double randomOut = fRand(dist + QPointF::dotProduct(lowestR+m_prevRandom[0], toAxis),
                                    dist + QPointF::dotProduct(selfR+m_prevRandom[0], toAxis));
            m_prevRandom[0] = (randomOut-dist) * toAxis;
            QPointF first = fromConnectorPos + randomOut * fromMove;
            turnPos.append(first);
            if (fromAddTo.x() == 0) turnPos.append(QPointF(toConnectorPos.x(), first.y()));
            else turnPos.append(QPointF(first.x(), toConnectorPos.y()));

        } else if (!fromSide && toSide) { // same direction + to true
            double randomSelf = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                                      QPointF::dotProduct(selfR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = (randomSelf) * fromAxis;
            QPointF first = fromConnectorPos + randomSelf * fromMove;
            turnPos.append(first);
            if (fromAddTo.x() == 0) turnPos.append(QPointF(toConnectorPos.x(), first.y()));
            else turnPos.append(QPointF(first.x(), toConnectorPos.y()));

        } else { // back to back
            double randomSelf = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                                      QPointF::dotProduct(selfR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = (randomSelf) * fromAxis;
            QPointF first = fromConnectorPos + randomSelf * fromMove;
            turnPos.append(first);

            //TODO: change lowestR -> bound
            double randomBtw = fRand(QPointF::dotProduct(lowestR+m_prevRandom[1], verMove),
                    verDist - QPointF::dotProduct(lowestR+m_prevRandom[1], verMove));
            m_prevRandom[1] = randomBtw * verMove;
            QPointF second = sign * randomBtw * verMove + first;
            turnPos.append(second);
            double randomOut = fRand(dist + QPointF::dotProduct(lowestR+m_prevRandom[2], toAxis),
                                    dist + QPointF::dotProduct(selfR+m_prevRandom[2], toAxis));
            m_prevRandom[2] = (randomOut-dist) * toAxis;
            QPointF third = fromConnectorPos + randomOut * (-fromMove);
            third = QPointF::dotProduct(third, fromAxis) * fromAxis + QPointF::dotProduct(second, verMove) * verMove;
            turnPos.append(third);
            if (fromAxis.x() == 0) turnPos.append(QPointF(toConnectorPos.x(), third.y()));
            else turnPos.append(QPointF(third.x(), toConnectorPos.y()));

        }

    } else {
        if (fromSide && toSide) { //face to face
            QPointF first = QPointF::dotProduct(fromConnectorPos, toAxis) * toAxis +
                    QPointF::dotProduct(toConnectorPos, fromAxis) * fromAxis;
            turnPos.append(first);

        } else if (fromSide && !toSide) { // same direction + from true
            //change fromAxis to bound
            double randomBtw = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                    dist - QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = randomBtw * fromAxis;
            QPointF first = fromConnectorPos + randomBtw * fromMove;
            turnPos.append(first);

            double randomOut = fRand(verDist + QPointF::dotProduct(lowestR+m_prevRandom[1], toAxis),
                                    verDist + QPointF::dotProduct(selfR+m_prevRandom[1], toAxis));
            m_prevRandom[1] = (randomOut-verDist) * toAxis;
            QPointF second = first + randomOut * toMove;
            turnPos.append(second);
            if (fromAxis.x() != 0) turnPos.append(QPointF(toConnectorPos.x(), second.y()));
            else turnPos.append(QPointF(second.x(), toConnectorPos.y()));

        } else if (!fromSide && toSide) { // same direction + to true
            double randomSelf = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                                      QPointF::dotProduct(selfR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = (randomSelf) * fromAxis;
            QPointF first = fromConnectorPos + randomSelf * fromMove;
            turnPos.append(first);

            double randomBtw = fRand(QPointF::dotProduct(lowestR+m_prevRandom[1], verMove),
                                    verDist - QPointF::dotProduct(lowestR+m_prevRandom[1], verMove));
            m_prevRandom[1] = (randomBtw) * verMove;
            QPointF second = first - randomBtw * toMove;
            turnPos.append(second);
            if (fromAxis.x() != 0) turnPos.append(QPointF(toConnectorPos.x(), second.y()));
            else turnPos.append(QPointF(second.x(), toConnectorPos.y()));

        }
        else { // back to back
            double randomSelf = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                                      QPointF::dotProduct(selfR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = (randomSelf) * fromAxis;

            QPointF first = fromConnectorPos + randomSelf * fromMove;
            turnPos.append(first);

            double randomOut = fRand(verDist + QPointF::dotProduct(lowestR+m_prevRandom[1], toAxis),
                                    verDist + QPointF::dotProduct(selfR+m_prevRandom[1], toAxis));
            m_prevRandom[1] = (randomOut - verDist) * toAxis;
            QPointF second = first + randomOut * toMove;
            turnPos.append(second);
            if (fromAxis.x() != 0) turnPos.append(QPointF(toConnectorPos.x(), second.y()));
            else turnPos.append(QPointF(second.x(), toConnectorPos.y()));


        }

    }

    QList<ItemBase *> wireList;
    ItemBase * nowWire = wireItem;
    foreach(QPointF p, turnPos) {
        Wire * w =  squareWire(nowWire, p);
        nowWire = qobject_cast<ItemBase*>(w);
        wireList.append(nowWire);
    }
    return wireList;
}

QList<ItemBase *> SketchWidget::arrangeWireCurve(ItemBase* wireItem, ItemBase * fromItem, const QString & fromConnectorID,
                              ItemBase * toItem, const QString & toConnectorID) {


    ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, fromItem->viewLayerPlacement());
    ConnectorItem * toConnectorItem = findConnectorItem(toItem, toConnectorID, toItem->viewLayerPlacement());
    if (!fromConnectorItem || !toConnectorItem ) return QList<ItemBase *>();

    QString from = fromItem->title();
    QString to = toItem->title();
    QPointF fromConnectorPos = fromConnectorItem->sceneAdjustedTerminalPoint(NULL);
    QPointF toConnectorPos = toConnectorItem->sceneAdjustedTerminalPoint(NULL);
    QPointF moveVector = toConnectorPos - fromConnectorPos;

    ConnectorArrange fromConnectorArrange = getConnectorArrange(fromItem, fromConnectorPos);
    ConnectorArrange toConnectorArrange = getConnectorArrange(toItem, toConnectorPos);

    QList<QPointF> moveList({QPointF(0, -1), QPointF(0, 1), QPointF(-1, 0), QPointF(1, 0)});
    QPointF fromMove = moveList[fromConnectorArrange];
    QPointF fromAxis(qAbs(fromMove.x()), qAbs(fromMove.y()));
    QPointF toMove = moveList[toConnectorArrange];
    QPointF toAxis(qAbs(toMove.x()), qAbs(toMove.y()));

    QPoint verMove(qAbs(fromMove.y()), qAbs(fromMove.x()));
    double sign = QPointF::dotProduct(moveVector, verMove) >= 0 ? 1 : -1;

    QPointF fromAddTo = fromMove+toMove;
    bool fromSide = QPointF::dotProduct(moveVector, fromMove) >= 0 ? true : false;
    bool toSide = QPointF::dotProduct(-moveVector, toMove) >= 0 ? true : false;
    QList<QPointF> turnPos;

    QPointF selfR(20, 20);
    QPointF lowestR(5, 5);
//    selfR += m_prevRandom;
//    lowestR += m_prevRandom;
    double dist = qAbs(QPointF::dotProduct(moveVector, fromAxis));
    double verDist = qAbs(QPointF::dotProduct(moveVector, verMove));

    if (fromAddTo.x() == 0 || fromAddTo.y() == 0) { // same axis
        if (fromSide && toSide) { //face to face
            double randomBtw = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                    dist - QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = randomBtw * fromAxis;
            QPointF first = fromConnectorPos + randomBtw * fromMove;
            turnPos.append(first);
            if (fromAddTo.x() == 0) turnPos.append(QPointF(toConnectorPos.x(), first.y()));
            else turnPos.append(QPointF(first.x(), toConnectorPos.y()));

        } else if (fromSide && !toSide) { // same direction + from true
            double randomOut = fRand(dist + QPointF::dotProduct(lowestR+m_prevRandom[0], toAxis),
                                    dist + QPointF::dotProduct(selfR+m_prevRandom[0], toAxis));
            m_prevRandom[0] = (randomOut-dist) * toAxis;
            QPointF first = fromConnectorPos + randomOut * fromMove;
            turnPos.append(first);
            if (fromAddTo.x() == 0) turnPos.append(QPointF(toConnectorPos.x(), first.y()));
            else turnPos.append(QPointF(first.x(), toConnectorPos.y()));

        } else if (!fromSide && toSide) { // same direction + to true
            double randomSelf = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                                      QPointF::dotProduct(selfR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = (randomSelf) * fromAxis;
            QPointF first = fromConnectorPos + randomSelf * fromMove;
            turnPos.append(first);
            if (fromAddTo.x() == 0) turnPos.append(QPointF(toConnectorPos.x(), first.y()));
            else turnPos.append(QPointF(first.x(), toConnectorPos.y()));

        } else { // back to back
            double randomSelf = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                                      QPointF::dotProduct(selfR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = (randomSelf) * fromAxis;
            QPointF first = fromConnectorPos + randomSelf * fromMove;
            turnPos.append(first);

            //TODO: change lowestR -> bound
            double randomBtw = fRand(QPointF::dotProduct(lowestR+m_prevRandom[1], verMove),
                    verDist - QPointF::dotProduct(lowestR+m_prevRandom[1], verMove));
            m_prevRandom[1] = randomBtw * verMove;
            QPointF second = sign * randomBtw * verMove + first;
            turnPos.append(second);
            double randomOut = fRand(dist + QPointF::dotProduct(lowestR+m_prevRandom[2], toAxis),
                                    dist + QPointF::dotProduct(selfR+m_prevRandom[2], toAxis));
            m_prevRandom[2] = (randomOut-dist) * toAxis;
            QPointF third = fromConnectorPos + randomOut * (-fromMove);
            third = QPointF::dotProduct(third, fromAxis) * fromAxis + QPointF::dotProduct(second, verMove) * verMove;
            turnPos.append(third);
            if (fromAxis.x() == 0) turnPos.append(QPointF(toConnectorPos.x(), third.y()));
            else turnPos.append(QPointF(third.x(), toConnectorPos.y()));

        }

    } else {
        if (fromSide && toSide) { //face to face
            QPointF first = QPointF::dotProduct(fromConnectorPos, toAxis) * toAxis +
                    QPointF::dotProduct(toConnectorPos, fromAxis) * fromAxis;
            turnPos.append(first);

        } else if (fromSide && !toSide) { // same direction + from true
            //change fromAxis to bound
            double randomBtw = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                    dist - QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = randomBtw * fromAxis;
            QPointF first = fromConnectorPos + randomBtw * fromMove;
            turnPos.append(first);

            double randomOut = fRand(verDist + QPointF::dotProduct(lowestR+m_prevRandom[1], toAxis),
                                    verDist + QPointF::dotProduct(selfR+m_prevRandom[1], toAxis));
            m_prevRandom[1] = (randomOut-verDist) * toAxis;
            QPointF second = first + randomOut * toMove;
            turnPos.append(second);
            if (fromAxis.x() != 0) turnPos.append(QPointF(toConnectorPos.x(), second.y()));
            else turnPos.append(QPointF(second.x(), toConnectorPos.y()));

        } else if (!fromSide && toSide) { // same direction + to true
            double randomSelf = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                                      QPointF::dotProduct(selfR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = (randomSelf) * fromAxis;
            QPointF first = fromConnectorPos + randomSelf * fromMove;
            turnPos.append(first);

            double randomBtw = fRand(QPointF::dotProduct(lowestR+m_prevRandom[1], verMove),
                                    verDist - QPointF::dotProduct(lowestR+m_prevRandom[1], verMove));
            m_prevRandom[1] = (randomBtw) * verMove;
            QPointF second = first - randomBtw * toMove;
            turnPos.append(second);
            if (fromAxis.x() != 0) turnPos.append(QPointF(toConnectorPos.x(), second.y()));
            else turnPos.append(QPointF(second.x(), toConnectorPos.y()));

        }
        else { // back to back
            double randomSelf = fRand(QPointF::dotProduct(lowestR+m_prevRandom[0], fromAxis),
                                      QPointF::dotProduct(selfR+m_prevRandom[0], fromAxis));
            m_prevRandom[0] = (randomSelf) * fromAxis;

            QPointF first = fromConnectorPos + randomSelf * fromMove;
            turnPos.append(first);

            double randomOut = fRand(verDist + QPointF::dotProduct(lowestR+m_prevRandom[1], toAxis),
                                    verDist + QPointF::dotProduct(selfR+m_prevRandom[1], toAxis));
            m_prevRandom[1] = (randomOut - verDist) * toAxis;
            QPointF second = first + randomOut * toMove;
            turnPos.append(second);
            if (fromAxis.x() != 0) turnPos.append(QPointF(toConnectorPos.x(), second.y()));
            else turnPos.append(QPointF(second.x(), toConnectorPos.y()));


        }

    }

    QList<ItemBase *> wireList;
    ItemBase * nowWire = wireItem;
    foreach(QPointF p, turnPos) {
        Wire * w =  squareWire(nowWire, p);
        nowWire = qobject_cast<ItemBase*>(w);
        wireList.append(nowWire);
    }
    return wireList;
}

SketchWidget::ConnectorArrange SketchWidget::getConnectorArrange(ItemBase * itemBase, QPointF point) {
    //Get 2 dianogals from rect of item
    //TODO: check what bounding rect returns?

    QRectF itemRect = itemBase->boundingRect();
    QPointF pos = itemBase->pos();
    bool diaTopLeft = isLeft(itemRect.topLeft()+pos, itemRect.bottomRight()+pos, point);
    bool diaTopRight = isLeft(itemRect.topRight()+pos, itemRect.bottomLeft()+pos, point);
    if (!diaTopLeft && diaTopRight) return ConnectorArrange::UP;
    if (diaTopLeft && !diaTopRight) return ConnectorArrange::DOWN;
    if (diaTopLeft && diaTopRight) return ConnectorArrange::LEFT;
    if (!diaTopLeft && !diaTopRight) return ConnectorArrange::RIGHT;
    return ConnectorArrange::UP;
}



//QPair<double, double> SketchWidget::calculateLineCoef(QRect itemRect) {

//    double a, b;
//    if (topLeft.x()-bottomRight.x() == 0) {
//        a = 0;
//        b = topLeft.y();

//    } else {
//        a = (topLeft.y()-bottomRight.y()) / (topLeft.x()-bottomRight-x());
//        b = -(bottomRight.x()*topLeft.y())+(topLeft.x()*bottomRight.y());
//        b /= (topLeft.x()-bottomRight.x());
//    }
//    return QPair<double, double>(a, b);
//}

void SketchWidget::selectModelSet(QSharedPointer<ModelSet> modelSet, bool transparent, bool next) {
    if (transparent) addToModelSet(modelSet, transparent, next);
    else {
        QUndoCommand* parentCommand = new TemporaryCommand(tr("Add modelSet %1").arg(modelSet->keyTitle()));
        newAddModelSetCommand(modelSet, parentCommand);
        m_undoStack->waitPush(parentCommand, 10);
    }
}

void SketchWidget::selectSetToSet(QSharedPointer<ModelSet> modelSet, QSharedPointer<SetConnection> setconnection, bool confirmSetConnection, bool transparent) {
    if (transparent) addSetToSet(modelSet, setconnection, confirmSetConnection, transparent);
    else if (confirmSetConnection) {
        newAddSetConnectionCommand(setconnection, NULL);
    } else {
        QUndoCommand* parentCommand = new TemporaryCommand(tr("Add modelSet / Connection %1").arg(modelSet->keyTitle()));
        newAddSetToSetCommand(modelSet, setconnection, confirmSetConnection, parentCommand);
        m_undoStack->waitPush(parentCommand, 10);
    }
}

AddModelSetCommand * SketchWidget::newAddModelSetCommand(QSharedPointer<ModelSet> modelSet, QUndoCommand * parentCommand) {
    return new AddModelSetCommand(this, modelSet, parentCommand);
}

AddSetToSetCommand * SketchWidget::newAddSetToSetCommand(QSharedPointer<ModelSet> modelSet, QSharedPointer<SetConnection> setConnection, bool confirmSetConnection, QUndoCommand * parentCommand) {
    return new AddSetToSetCommand(this, modelSet, setConnection, confirmSetConnection, parentCommand);
}

AddSetConnectionCommand * SketchWidget::newAddSetConnectionCommand(QSharedPointer<SetConnection> setConnection, QUndoCommand *parentCommand) {
    bool newParent = false;
    if (parentCommand == NULL) {
        QSharedPointer<ModelSet> from = setConnection->getFromModelSet();
        QSharedPointer<ModelSet> to = setConnection->getToModelSet();
        parentCommand = new TemporaryCommand(tr("Add connection between %1, %2").arg(from->keyTitle()).arg(to->keyTitle()));
        newParent = true;
    }
    AddSetConnectionCommand * addSetConnectionCommand = new AddSetConnectionCommand(this, setConnection, parentCommand);
    if (newParent) m_undoStack->waitPush(parentCommand, 10);

    return addSetConnectionCommand;
}

void SketchWidget::addToModelSet(QSharedPointer<ModelSet> modelSet, bool transparent, bool next) {
    findKeyItem(modelSet);
    
    addModelSet(modelSet, transparent);
    addSetConnection(modelSet->breadboardConnection(), transparent);
//    if (!m_breadBoardGnd && !m_breadBoardModelSet->breadboardConnection().isNull()
//            && modelSet->isMicrocontroller()) {
//        addSetConnection(m_breadBoardModelSet->breadboardConnection(), transparent);
//        m_breadBoardGnd = true;
//    } else if (!transparent) {
//        setOpacity(m_breadBoardModelSet->breadboardConnection());
//    }
    QSharedPointer<SetConnection> breadBoardSetConnection = modelSet->breadboardConnection();
    if (!breadBoardSetConnection.isNull())
        modelSet->addBreadboardConnection(breadBoardSetConnection);

    if (!transparent || (modelSet->single() && breadBoardSetConnection.isNull() && next)) {
        if (!m_savedModelSet.contains(modelSet)) m_savedModelSet.append(modelSet);
        confirmSelect(modelSet);
    } else {
        m_prevModelSet = modelSet;
    }
}

void SketchWidget::calculatePos(QSharedPointer<ModelSet> modelSet) {
    if (modelSet->isMicrocontroller()) {
        QList<QSharedPointer<ModelSet>> mList = getMicrocontroller();
        if (mList.length() == 0) m_tempPoint = QPointF(100, 300);
        else m_tempPoint = QPoint(100, -100);
    }
    else {


    }
}

void SketchWidget::addSetToSet(QSharedPointer<ModelSet> modelSet, QSharedPointer<SetConnection> setconnection, bool confirmSetConnection, bool transparent) {

    QSharedPointer<ModelSet> temp = m_prevModelSet;
    QSharedPointer<ModelSet> from = setconnection->getFromModelSet();
    findKeyItem(from);
    if (from->keyItem()!=NULL) {
        m_tempPoint = from->keyItem()->getViewGeometry().loc()+from->keyItem()->boundingRect().center();
        m_tempPoint = m_tempPoint-QPoint(120, 0);
    }
    addModelSet(modelSet, transparent);
    addSetConnection(modelSet->breadboardConnection(), transparent);
//    if (!m_breadBoardGnd && !m_breadBoardModelSet->breadboardConnection().isNull() && modelSet->isMicrocontroller()) {
//        addSetConnection(m_breadBoardModelSet->breadboardConnection(), transparent);
//        m_breadBoardGnd = true;
//    } else if (!transparent) {
//        setOpacity(m_breadBoardModelSet->breadboardConnection());
//    }
    //if (transparent || temp!= modelSet)
    //TODO:
    if (confirmSetConnection) addSetConnection(setconnection, transparent);
    if (!modelSet->breadboardConnection().isNull())
        modelSet->addBreadboardConnection(modelSet->breadboardConnection());
    //if (!confirmSetConnection && )
    modelSet->addSetConnection(setconnection);
    if (!transparent && confirmSetConnection) {
        //setOpacity(modelSet);
        //setOpacity(modelSet->setConnection());
        confirmSelect(modelSet);
    } else if (!transparent) {
        //TODO: if modelset is breadboard
        m_prevModelSet.clear();
        QSharedPointer<ModelSet> fromModelSet = setconnection->getFromModelSet();
        QSharedPointer<ModelSet> toModelSet = setconnection->getToModelSet();
        QList<QPair<QString, QString>> connectedPair = fromModelSet->getConnectedPairWithModelSet(toModelSet);
        AutoCompleter::getSuggestionConnection(fromModelSet, toModelSet, connectedPair, this);
    } else {
        m_prevModelSet = modelSet;
    }

}

void SketchWidget::addSetConnection(QSharedPointer<SetConnection> setconnection, bool transparent) {
    if (setconnection.isNull() || setconnection->isConfirm()) return;
    if (m_prevModelSet != NULL) {
        if (setconnection == m_prevModelSet->setConnection() || setconnection == m_prevModelSet->breadboardConnection()) {
            if (!transparent) {
                setOpacity(setconnection);
            }
            return;
        }
    }
    removePrevSetConnection(false);
    QSharedPointer<ModelSet> from = setconnection->getFromModelSet();
    QSharedPointer<ModelSet> to = setconnection->getToModelSet();
    if (from.isNull() || to.isNull()) return;
    m_prevRandom = QList<QPointF>{QPointF(0, 0),QPointF(0, 0),QPointF(0, 0)};
    //findKeyItem(from);
    QList<QString> usedConnectorID;

    QSet<QString> connectedSet = from->getConnectedNameWithModelSet(m_breadBoardModelSet);
    QList<SetConnection::Connection> connectionList = setconnection->getConnectionList();
    foreach(SetConnection::Connection c, connectionList) {
        QPair<ItemBase *, QString> p1 = from->getItemAndCID(c.fromTerminal);
        QPair<ItemBase *, QString> p2;
        QString toPintype = to->getPinType(c.toTerminal);
        QList<QPair<ModelSet::Terminal, QString>> pintypeT = to->getPinTypeTerminal(toPintype);
        QList<QPair<ModelSet::Terminal, QString>> breadboardT = m_breadBoardModelSet->getPinTypeTerminal(toPintype);
        QString breadboardPin = "";
        bool nearestBreadBoard = false;
        if (!to->breadboardConnection().isNull()) {
            breadboardPin = to->breadboardConnection()->getConnectedTo(1, c.toTerminal);
        }
        if (connectedSet.contains(c.fromTerminal)){
            continue;
        }

        QList<QPair<ItemBase *, QString>> p1List;
        QList<QPair<ItemBase *, QString>> p2List;

        if (to->isMicrocontroller() && from != m_breadBoardModelSet && breadboardPin != "") {
            QList<QString> vccConnectorIDList;
            vccConnectorIDList.append(breadboardPin);
            //TODO: findBreadBoardNearest
            ConnectorItem * ci = p1.first->findConnectorItemWithSharedID(p1.second);
            QString vccConnectorID;
            if (ci) {
                nearestBreadBoard = true;
                vccConnectorID = findBreadBoardNearest(ci->sceneAdjustedTerminalPoint(NULL), vccConnectorIDList, vccConnectorIDList, false);
            } else vccConnectorID = findBreadBoardUnused(vccConnectorIDList, vccConnectorIDList, false);
            p2 = QPair<ItemBase *, QString>(m_breadBoardModelSet->keyItem(), vccConnectorID);
            p2List.append(p2);
            p1List.append(p1);
        }
//        else if (to->isMicrocontroller() && breadboardT.length() > 0 && from != m_breadBoardModelSet) {
//            // if connect to microcontroller and already has pin type
//            ModelSet::Terminal t = breadboardT[0].first;
//            QString vccConnectorID = t.connectorID;
//            QList<QString> vccConnectorIDList;
//            vccConnectorIDList.append(vccConnectorID);
//            vccConnectorID = findBreadBoardUnused(vccConnectorIDList, vccConnectorIDList, false);
//            p2 = QPair<ItemBase *, QString>(m_breadBoardModelSet->keyItem(), vccConnectorID);
//            p2List.append(p2);
//        }
        else if (to->isMicrocontroller() && pintypeT.length() > 0 && from != m_breadBoardModelSet) {

            QString connectorID = pintypeT[0].first.connectorID;
            foreach(TerminalStringPair tpair, pintypeT) {
                if (usedConnectorID.contains(tpair.first.connectorID)) continue;
                usedConnectorID.append(tpair.first.connectorID);
                ConnectorItem * connectorItem = to->keyItem()->findConnectorItemWithSharedID(tpair.first.connectorID);
                QList<QPointer<ConnectorItem>> connectorItemList = connectorItem->connectedToItems();
                if (connectorItemList.length() != 0) continue;
                else {
                    connectorID = tpair.first.connectorID;
                    break;
                }
            }
            p2 = QPair<ItemBase *, QString>(to->keyItem(), connectorID);
            p2List.append(p2);
            p1List.append(p1);

        }
        else if (to->isMicrocontroller() || from != m_breadBoardModelSet) {
            p2 = to->getItemAndCID(c.toTerminal);
            p2List.append(p2);
            p1List.append(p1);
        } else { // from == breadBoard and to != micro

            p2List = to->getItemAndCIDAll(c.toTerminal);
            if (p2List.length() > 0) {
                p2 = p2List[0];
                foreach(ItemBaseStringPair isp, p2List) {
                    QList<QString> vccConnectorIDList;
                    vccConnectorIDList.append(c.fromTerminal);

                    ConnectorItem * ci = isp.first->findConnectorItemWithSharedID(isp.second);
                    QString vccConnectorID;
                    if (ci) {
                        nearestBreadBoard = true;
                        vccConnectorID = findBreadBoardNearest(ci->sceneAdjustedTerminalPoint(NULL), vccConnectorIDList, vccConnectorIDList, false);
                    } else vccConnectorID = findBreadBoardUnused(vccConnectorIDList, vccConnectorIDList, false);
                    p1 = QPair<ItemBase*, QString>(m_breadBoardModelSet->keyItem(), vccConnectorID);
                    p1List.append(p1);
                }

            }
            else p2 = QPair<ItemBase*, QString>();
        }

        if (p1.first == NULL|| p2.first == NULL || p1.second == "" ||  p2.second == "") continue;
        DebugDialog::debug(QString("p1: %1").arg(p1.first->title()));
        DebugDialog::debug(QString("p2: %1").arg(p2.first->title()));

        //TODO: change wire connection;
        int i = 0;
        foreach(ItemBaseStringPair p, p2List) {
            p1 = p1List[i];
            i++;
            ItemBase * wire = addSetWire(p1.first, p1.second, p.first, p.second, transparent);
            ItemBase * finalWire = wire;
            if (!wire) continue;
            QList<ItemBase *> newWireList;
            if (!nearestBreadBoard) {

                newWireList = arrangeWire(wire, p1.first, p1.second, p.first, p.second);
            }
            ViewLayer::ViewLayerPlacement place;
            if (c.changeColor) {
                Wire * w = qobject_cast<Wire *>(wire);
                if (w) {
                    if (c.color == QColor(255, 0, 0)) w->setColorString("red", 1, true);
                    else if (c.color == QColor(0, 0, 0)) w->setColorString("black", 1, true);
                    updateInfoView();
                }

                //w->setColor(c.color, 1);
                place = ViewLayer::specFromID(w->connector1()->attachedToViewLayerID());
            }
            //p1.first(item) -> id?, p1.second
            wire->setModelSet(to);
            setconnection->appendWireList(wire);


            foreach(ItemBase * wireI, newWireList) {
                if (c.changeColor) {
                    Wire * w = qobject_cast<Wire *>(wireI);
                    //w->setColor(c.color, 1);
                    if (w) {
                        if (c.color == QColor(255, 0, 0)) w->setColorString("red", 1, true);
                        else if (c.color == QColor(0, 0, 0)) w->setColorString("black", 1, true);
                        updateInfoView();
                    }

                }
                wireI->setModelSet(to);
                wireI->setOpacity(wire->opacity());
                setconnection->appendWireList(wireI);
                finalWire = wireI;

            }
            if (finalWire == wire) {
                setconnection->insertWireConnection(wire, p1.first->id(), p1.second, p.first->id(), p.second);
            } else {
                setconnection->insertWireConnection(wire, p1.first->id(), p1.second, -1, "");
                setconnection->insertWireConnection(finalWire, -1, "", p.first->id(), p.second);
                changeConnection(p1.first->id(), p1.second, wire->id(), "connector0", place, true, false, true);
                changeConnection(p.first->id(), p.second, finalWire->id(), "connector1", place, true, false, true);

            }
            //setconnection->appendWireList(newWire);
            //TODO: wire store which set connection it belongs to
        }

	}
    if (!transparent) setconnection->setConfirm();

}

void SketchWidget::addModelSet(QSharedPointer<ModelSet> modelSet, bool transparent) {

    //findKeyItem(modelSet);

    if (modelSet == m_prevModelSet) {
       //removePrevSetConnection(false);
        if (!transparent) {
            if (!modelSet->isConfirm()) m_savedModelSet.append(modelSet);
            setOpacity(modelSet);
            setOpacity(modelSet->breadboardConnection());
        }
        return;
    }    
	removePrevModelSet();

    if (modelSet.isNull()) return;

    QList<ModelSet::TerminalPair> connectionList = modelSet->getConnections();
    QString keyLabel = modelSet->keyLabel();


    foreach(ModelSet::TerminalPair c, connectionList) {
        ModelSet::Terminal from = c.first;
        ModelSet::Terminal to = c.second;
        ItemBase * fromItem = modelSet->getItem(modelSet->genLabelHashKey(from));
        if (fromItem == NULL) {
            //if (modelSet->isMicrocontroller() && from.moduleID == modelSet->keyModuleID())
            //    m_tempPoint = QPointF(100, 300);
            calculatePos(modelSet);
            fromItem = addSetItem(m_tempPoint, from.moduleID, transparent);

            if (fromItem == NULL) {
                continue;
            }
            //adjust item pos
            //adjustItemPos()
            if (from.label == modelSet->keyLabel()){
                modelSet->setKeyId(fromItem->id());
                modelSet->setKeyItem(fromItem);
            }
            modelSet->insertLabelHash(modelSet->genLabelHashKey(from), fromItem);
            modelSet->addItem(fromItem);
            fromItem->setModelSet(modelSet);
        }
        if (to.title == "NULL" || to.title == "") continue;
        ItemBase * toItem = modelSet->getItem(modelSet->genLabelHashKey(to));
        if (toItem == NULL) {
            toItem = addSetItem(fromItem, from.connectorID, to.moduleID, to.connectorID, transparent);
            if (toItem == NULL) {
                continue;
            }
            if (to.label == modelSet->keyLabel()){
                modelSet->setKeyId(toItem->id());
                modelSet->setKeyItem(toItem);
            }
            modelSet->insertLabelHash(modelSet->genLabelHashKey(to), toItem);
            modelSet->addItem(toItem);
            toItem->setModelSet(modelSet);

        }
//        else {

//            //adjust pos?





//        }

        //TODO: don't wire if vcc/ gnd
        if (from.type == to.type && (ModelSet::pinEqual("GND", from.type) != "" || ModelSet::pinEqual("VCC", from.type) != ""))
            continue;

        DebugDialog::debug(QString("from: %1, to: %2").arg(fromItem->title()).arg(toItem->title()));
        ItemBase * wire = addSetWire(fromItem, from.connectorID, toItem, to.connectorID, transparent);
        if (!wire) continue;
        //Wire * newW = squareWire(wire);
        //if (!newW) continue;
        //ItemBase * newWire = qobject_cast<ItemBase *>(newW);

        wire->setModelSet(modelSet);
        //newWire->setModelSet(modelSet);
        //newWire->setOpacity(wire->opacity());
        modelSet->insertWireConnection(wire, QPair<ModelSet::Terminal, ModelSet::Terminal>(from, to));
        //modelSet->insertWire
        //Wire * w = qobject_cast<Wire *>(wire);
        modelSet->addItem(wire);
        //modelSet->addItem(newWire);
        //TODO: arduino
    }

    completeSuggestion(modelSet, transparent);
    if (!transparent) modelSet->setConfirm();

}

void SketchWidget::findKeyItem(QSharedPointer<ModelSet> modelSet) {
    //TODO: change to command type
    if (modelSet.isNull()) return;
    ItemBase * keyItem = modelSet->keyItem();
    long id = modelSet->keyId();
    keyItem = findItem(id);
    if (keyItem != NULL) {
        modelSet->setKeyItem(keyItem);
        keyItem->setModelSet(modelSet);
    }

}

void SketchWidget::deleteModelSet(QSharedPointer<ModelSet> modelSet) {
    if (modelSet.isNull()) return;
    QList<ItemBase *> itemList = modelSet->getItemList();
    foreach(ItemBase * itemBase, itemList) {
        if (itemBase == modelSet->keyItem()) {
            modelSet->setKeyItem(NULL);
        }
        deleteItem(itemBase, true, false, false);
    }
    modelSet->emptyItemList();
    modelSet->setConfirm(false);
    if (modelSet->keyItem() != NULL) {
        modelSet->keyItem()->setModelSet(QSharedPointer<ModelSet>(NULL));
    }

    // empty list
    deleteSetConnection(modelSet->setConnection());
    deleteSetConnection(modelSet->breadboardConnection());

    if (modelSet != m_prevModelSet && !m_prevModelSet.isNull()) {
        deleteModelSet(m_prevModelSet);

    }
    m_prevModelSet.clear();
    if (m_savedModelSet.contains(modelSet)) {
        m_savedModelSet.removeOne(modelSet);
    }
    if (m_breadBoardModelSet == modelSet) {
        m_breadBoardModelSet.clear();
    }
    AutoCompleter::clearRecommend();
}

void SketchWidget::deleteSetConnection(QSharedPointer<SetConnection> setConnection) {
    if (setConnection.isNull()) return;
    QList<ItemBase *> itemList;
    itemList.append(setConnection->getWireList());
    foreach(ItemBase * itemBase, itemList) {
        deleteItem(itemBase, false, false, false);

    }
    setConnection->emptyWireList();
    if (setConnection->isConfirm()) {
        setConnection->setConfirm();
    }
    QSharedPointer<ModelSet> fromModelSet = setConnection->getFromModelSet();
    QSharedPointer<ModelSet> toModelSet = setConnection->getToModelSet();
    fromModelSet->deleteSetConnection(setConnection);
    toModelSet->deleteSetConnection(setConnection);

}

void SketchWidget::removePrevModelSet() {

    if (m_prevModelSet.isNull()) return;
    if (!m_prevModelSet->isConfirm()) {
        QList<ItemBase *> itemList = m_prevModelSet->getItemList();
        //TODO: check if correct
        findKeyItem(m_prevModelSet);
        m_prevModelSet->keyItem()->setModelSet(QSharedPointer<ModelSet>(NULL));
        foreach(ItemBase * itemBase, itemList) {
            if (itemBase == m_prevModelSet->keyItem()) {
                m_prevModelSet->setKeyItem(NULL);
            }
            deleteItem(itemBase, false, true, false);

//            //itemBase->removeLayerKin();
//            this->scene()->removeItem(itemBase);
//            if (itemBase->modelPart()) {
//                delete itemBase->modelPart();
//            }
//            delete itemBase;
        }
        //TODO: m_prevModelSet Key Item delete?
        m_prevModelSet->emptyItemList();

    }
    removePrevSetConnection(true);
    m_prevModelSet.clear();


}

void SketchWidget::removePrevSetConnection(bool removeBreadboard) {
    if (m_prevModelSet.isNull()) return;
    QSharedPointer<SetConnection> setConnection = m_prevModelSet->setConnection();
    QSharedPointer<SetConnection> breadboardConnection = m_prevModelSet->breadboardConnection();
    QSharedPointer<SetConnection> breadboardOwnConnection = m_breadBoardModelSet->breadboardConnection();
    if (setConnection.isNull() && breadboardConnection.isNull() && breadboardOwnConnection.isNull()) return;
    QList<ItemBase *> itemList;
    if (!setConnection.isNull() && !setConnection->isConfirm()) {
        itemList.append(setConnection->getWireList());
        setConnection->emptyWireList();
    }
    if (!breadboardConnection.isNull() && removeBreadboard && !breadboardConnection->isConfirm()) {
        itemList.append(breadboardConnection->getWireList());
        breadboardConnection->emptyWireList();
    }
    if (!breadboardOwnConnection.isNull() && removeBreadboard && !breadboardOwnConnection->isConfirm() && m_breadBoardGnd) {
        itemList.append(breadboardOwnConnection->getWireList());
        breadboardOwnConnection->emptyWireList();
        m_breadBoardGnd = false;
    }
    foreach(ItemBase * itemBase, itemList) {
        //itemBase->removeLayerKin();
        deleteItem(itemBase, false, false, false);

//        this->scene()->removeItem(itemBase);
//        if (itemBase->modelPart()) {
//            delete itemBase->modelPart();
//        }
//        delete itemBase;
    }
    //if (!setConnection.isNull()) setConnection->emptyWireList();
    //if (!breadboardConnection.isNull() && removeBreadboard && !breadboardConnection->isConfirm()) breadboardConnection->emptyWireList();
    m_prevModelSet->clearSetConnection();

}

ItemBase* SketchWidget::addSetWire(ItemBase * fromItem, const QString & fromConnectorID, ItemBase * toItem, const QString & toConnectorID, bool transparent) {
    //DebugDialog::debug(QString("%1").arg(fromItem->title()));
    // ItemBase * itemBase = findItem(fromItem->id());
    // if (itemBase == NULL) {
    //     return NULL;
    // }

    ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, fromItem->viewLayerPlacement());
    if (fromConnectorItem == NULL) return NULL;

	ModelPart * wirePart = m_referenceModel->retrieveModelPart(QString("WireModuleID"));
	long wireID = ItemBase::getNextID();
	ViewGeometry viewGeometryWire;
	QLineF line;
    //QList<ConnectorItem *> exclude;
    //ConnectorItem * fromOverConnectorItem = fromConnectorItem->findConnectorUnder(true, true, exclude, true, fromConnectorItem);
    //QPointF fromOverPos = fromOverConnectorItem->sceneAdjustedTerminalPoint(NULL);

    ConnectorItem * toConnectorItem = findConnectorItem(toItem, toConnectorID, toItem->viewLayerPlacement());
    QPointF fromConnectorPos = fromConnectorItem->sceneAdjustedTerminalPoint(NULL);
    QPointF toConnectorPos = toConnectorItem->sceneAdjustedTerminalPoint(NULL);

    line.setLine(0, 0, -fromConnectorPos.x()+toConnectorPos.x(), -fromConnectorPos.y()+toConnectorPos.y());

    viewGeometryWire.setLoc(fromConnectorPos);
	viewGeometryWire.setLine(line);
    ItemBase * wire = addItemAuxTemp(wirePart, defaultViewLayerPlacement(wirePart), viewGeometryWire, wireID, true, m_viewID, transparent);
   	if (wire == NULL) return NULL;
    if (transparent) wire->setOpacity(0.5);

    changeConnection(fromItem->id(), fromConnectorID, wireID, "connector0", ViewLayer::specFromID(fromConnectorItem->attachedToViewLayerID()), true, false, false);
    changeConnection(toItem->id(), toConnectorID, wireID, "connector1",ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()), true, false, false);

    return wire;
}

ItemBase * SketchWidget::addSetItem(QPointF pos, QString & toModuleID, bool transparent) {

	toModuleID = checkDroppedModuleID(toModuleID);
	ModelPart * modelPart = m_referenceModel->retrieveModelPart(toModuleID);
    if (modelPart ==  NULL) return NULL;
    if (!canDropModelPart(modelPart)) return NULL;


	ViewGeometry viewGeometry;
    //QPointF fromOverPos = connectorItem->sceneAdjustedTerminalPoint(NULL);

	long toID = ItemBase::getNextID();
	bool doConnectors = true;
    ItemBase *toItem = addItem(toModuleID, defaultViewLayerPlacement(modelPart), BaseCommand::CrossView, viewGeometry, toID, -1, NULL);
            //addItemAuxTemp(modelPart, defaultViewLayerPlacement(modelPart), viewGeometry, toID, doConnectors, m_viewID, transparent);

    //ItemBase *toItem = addItemAuxTemp(modelPart, defaultViewLayerPlacement(modelPart), viewGeometry, toID, doConnectors, m_viewID, transparent);

    toItem->setPos(pos-toItem->boundingRect().center());

    if (transparent) toItem->setOpacity(0.5);

    return toItem;

}


ItemBase * SketchWidget::addSetItem(ItemBase * fromItem, const QString & fromConnectorID, QString toModuleID, const QString & toConnectorID, bool transparent) { // TODO: add offset, paras
	
	//toModuleID = QString("ResistorModuleID");

    //QString toConnectorID2 = QString("connector1");
	// get from item
    ConnectorItem * fromConnectorItem = findConnectorItem(fromItem, fromConnectorID, fromItem->viewLayerPlacement());
    if (fromConnectorItem == NULL) return NULL; // for now

	//check connection item under
	QList<ConnectorItem *> exclude;
    ConnectorItem * fromOverConnectorItem = fromConnectorItem->findConnectorUnder(true, true, exclude, true, fromConnectorItem);
    ConnectorItem * connectorItem = fromOverConnectorItem == NULL ? fromConnectorItem : fromOverConnectorItem;
    //TODO fromOverConnectorItem == NULL?
    if (connectorItem != NULL) {
        //DebugDialog::debug(QString("fromOverConnectorItem:%1").arg(fromOverConnectorItem->connectorSharedID()));
        //fromOverConnectorItem->setOverConnectorItem(NULL);   // clean up
		toModuleID = checkDroppedModuleID(toModuleID);
		ModelPart * modelPart = m_referenceModel->retrieveModelPart(toModuleID);
        if (modelPart ==  NULL) return NULL;
        if (!canDropModelPart(modelPart)) return NULL;


		ViewGeometry viewGeometry;
        QPointF fromOverPos = connectorItem->sceneAdjustedTerminalPoint(NULL);

		long toID = ItemBase::getNextID();
		bool doConnectors = true;
		ItemBase *toItem = addItemAuxTemp(modelPart, defaultViewLayerPlacement(modelPart), viewGeometry, toID, doConnectors, m_viewID, true);
        
        ConnectorItem * toConnectorItem = findConnectorItem(toItem, toConnectorID, toItem->viewLayerPlacement());
		QPointF toPos = toItem->getViewGeometry().loc();
		QPointF toConnectorPos = toConnectorItem->sceneAdjustedTerminalPoint(NULL);

		if (modelPart->family() != "microcontroller board (arduino)") {

            ConnectorArrange fromConnectorArrange = getConnectorArrange(fromItem, fromOverPos);
            ConnectorArrange toConnectorArrange = getConnectorArrange(toItem, toConnectorPos);
            QList<QPointF> moveList({QPointF(0, -1), QPointF(0, 1), QPointF(-1, 0), QPointF(1, 0)});
            QPointF fromMove = moveList[fromConnectorArrange];

            QPointF offset(0, 0);
            QPointF terminalOffset = toPos-toConnectorPos;
            QPointF fromCenter = fromItem->getViewGeometry().loc()+fromItem->boundingRect().center();
//            if (fromCenter.x()- fromOverPos.x() > 0) {
//                offset.setX(-27);
//			} else {
//                offset.setX(27);
//			}
            offset.setY(9);
            //offset = fromMove * 18;

            /********************* NOT GOOD HERE ***************************/

            if (toItem->moduleID() == "ResistorModuleID" ) {
                if (fromItem->moduleID() == "5mmColorLEDModuleID") {
                    offset.setY(9);
                }
            }
            if (fromItem->title() == "LCD screen") {
                if (toItem->title() == "Rotary Potentiometer (Small)") {
                    offset.setY(-81);
                    offset.setX(-45);
                }


            }










            /*****************************************************************/

		//if (toModuleID != "arduino_Uno_Rev3(fix)") {
			toItem->setPos(fromOverPos+terminalOffset+offset);
        }
        if (transparent) toItem->setOpacity(0.5);

        //changeConnection(fromOverConnectorItem->attachedTo()->id(), fromOverConnectorItem->connectorSharedID(), wireID, "connector0", ViewLayer::specFromID(fromOverConnectorItem->attachedToViewLayerID()), true, false, false);
        //changeConnection(toID, toConnectorID, wireID, "connector1",ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()), true, false, false);
		//extendChangeConnectionCommand(BaseCommand::CrossView, toConnectorItem, wire->connector0, ViewLayer::specFromID(toConnectorItem->attachedToViewLayerID()), true, parentCommand);
        return toItem;
	}
    return NULL;

}

void SketchWidget::getSuggestionConnection(Wire *wire, ConnectorItem * to) {
    if (!m_autoComplete) return;
    if (to == NULL) return;

    ConnectorItem * toConnectorItem = NULL;
    DebugDialog::debug(QString("to : %1").arg(to->attachedTo()->title()));
    if (to->attachedTo() != m_breadBoardModelSet->keyItem()) {
        toConnectorItem = to;
    } else {
        QList<ConnectorItem *> connectorItems;
        connectorItems.append(to);
        ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);
        foreach(ConnectorItem * citem, connectorItems) {
            ItemBase * item = citem->attachedTo();
            QString s = citem->attachedTo()->title();
            if (item != m_breadBoardModelSet->keyItem() && item != wire) {
                toConnectorItem = citem;
                //break;
            }
        }
    }

    ConnectorItem * connector0 = wire->connector0();
    ConnectorItem * fromConnectorItem = findConnectorItemTo(connector0, toConnectorItem);
    //ConnectorItem * connector1 = wire->connector1();
    //ConnectorItem * toConnectorItem = findConnectorItemTo(connector1, NULL);
    if (fromConnectorItem == NULL || toConnectorItem == NULL) return;


    DebugDialog::debug(QString("%1 attached to : %2").arg(fromConnectorItem->connectorSharedID()).arg(fromConnectorItem->attachedTo()->title()));
    DebugDialog::debug(QString("attached to : %1").arg(toConnectorItem->attachedTo()->title()));

    foreach (QSharedPointer<ModelSet> ms, m_savedModelSet) {
        findKeyItem(ms);
    }

    QSharedPointer<ModelSet> fromModelSet = fromConnectorItem->attachedTo()->modelSet();
    QSharedPointer<ModelSet> toModelSet = toConnectorItem->attachedTo()->modelSet();
    if (fromModelSet.isNull() || toModelSet.isNull()) return;

    //QSharedPointer<SetConnection> setconnection = getSetConnectionWithModelSet(QSharedPointer<ModelSet> modelset);
    QSharedPointer<SetConnection> setconnection = QSharedPointer<SetConnection>(new SetConnection(fromModelSet, toModelSet));
    QString fromName = fromModelSet->getTerminalName(fromConnectorItem->attachedTo()->moduleID(), fromConnectorItem->connectorSharedID());
    QString toName = toModelSet->getTerminalName(toConnectorItem->attachedTo()->moduleID(), toConnectorItem->connectorSharedID());
    setconnection->appendConnection(fromName, toName);
    fromModelSet->appendSetConnectionList(0, setconnection);
    toModelSet->appendSetConnectionList(1, setconnection);
    QList<QPair<QString, QString>> connectedPair = fromModelSet->getConnectedPairWithModelSet(toModelSet);
    AutoCompleter::getSuggestionConnection(fromModelSet, toModelSet, connectedPair, this);

}

ConnectorItem * SketchWidget::findConnectorItemTo(ConnectorItem * connectorItem, ConnectorItem * excludeConnector){
    ConnectorItem * fromConnectorItem = NULL;
    QList<ConnectorItem *> exclude;
    if (excludeConnector != NULL) exclude.append(excludeConnector);
    ConnectorItem * under = connectorItem->findConnectorUnder(true, true, exclude, false, NULL);
    //if (under) {
    QList<ConnectorItem *> connectorItems;
    connectorItems.append(connectorItem);
    ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);
    foreach(ConnectorItem * citem, connectorItems) {
        ItemBase * item = citem->attachedTo();
        QString s = citem->attachedTo()->title();
        if (item != m_breadBoardModelSet->keyItem() && item != connectorItem->attachedTo()) {
            fromConnectorItem = citem;
            //break;
        }
    }
    //}
   // under = connectorItem;




//    if (under->attachedTo() == m_breadBoardModelSet->keyItem()) {
//        QList<ConnectorItem *> connectorItems;
//        connectorItems.append(connectorItem);
//        ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);
//        foreach(ConnectorItem * citem, connectorItems) {
//            ItemBase * item = citem->attachedTo();
//            QString s = citem->attachedTo()->title();
//            if (item != m_breadBoardModelSet->keyItem() && item != connectorItem->attachedTo()) {
//                fromConnectorItem = citem;
//                //break;
//            }
//        }
//    } else {
//        fromConnectorItem = under;
//    }
    return fromConnectorItem;

}

QList<QSharedPointer<ModelSet>> SketchWidget::getMicrocontroller() {
    QList<QSharedPointer<ModelSet>> mcuList;
    foreach(QSharedPointer<ModelSet> modelset, m_savedModelSet) {
        if (modelset->isMicrocontroller())
            mcuList.append(modelset);
        //if (modelset->isMicrocontroller()) return modelset;
    }
    //return QSharedPointer<ModelSet>(NULL);
    return mcuList;
}

QString SketchWidget::findBreadBoardUnused(QList<QString> connectorIDList, QList<QString> excludeConnectorIDList, bool checkConnected) {
    if (m_breadBoardModelSet.isNull()) return "";
    
    ItemBase * breadboard = m_breadBoardModelSet->keyItem();
    foreach(QString s, connectorIDList) {

        ConnectorItem * connectorItem = breadboard->findConnectorItemWithSharedID(s);
        QList<QPointer<ConnectorItem>> connectorItemList = connectorItem->connectedToItems();
        if (connectorItemList.length() != 0 && checkConnected) continue;
        else return s;
        //CHECK HERE! For speed upXD

        bool connected = false;
        QList<ConnectorItem *> connectorItems;
        connectorItems.append(connectorItem);
        ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);
        foreach(ConnectorItem * citem, connectorItems) {
            if (excludeConnectorIDList.contains(citem->connectorSharedID())) continue;
            connectorItemList = citem->connectedToItems();
            if (connectorItemList.length() == 0) return citem->connectorSharedID();
            if (connectorItemList.length() != 0 && checkConnected) {
                connected = true;
                break;
            }
        }
        if (!connected && checkConnected) return s;

    }
    return "";
}

QString SketchWidget::findBreadBoardNearest(QPointF pos, QList<QString> connectorIDList, QList<QString> excludeConnectorIDList, bool checkConnected) {
    if (m_breadBoardModelSet.isNull()) return "";
    QList<ConnectorItem * > breadboardList;
    ItemBase * breadboard = m_breadBoardModelSet->keyItem();
    foreach(QString s, connectorIDList) {

        ConnectorItem * connectorItem = breadboard->findConnectorItemWithSharedID(s);
        QList<QPointer<ConnectorItem>> connectorItemList = connectorItem->connectedToItems();
        if (connectorItemList.length() != 0 && checkConnected) continue;

        bool connected = false;
        QList<ConnectorItem *> connectorItems;
        connectorItems.append(connectorItem);
        ConnectorItem::collectEqualPotential(connectorItems, true, ViewGeometry::NoFlag);
        foreach(ConnectorItem * citem, connectorItems) {
            if (excludeConnectorIDList.contains(citem->connectorSharedID())) continue;
            connectorItemList = citem->connectedToItems();
            if (connectorItemList.length() == 0) continue;
            if (connectorItemList.length() != 0 && checkConnected) {
                connected = true;
                break;
            }
        }
        if (!connected) {
            breadboardList.append(connectorItems);
        }

    }
    double minDist = 100000000;
    ConnectorItem * minC = NULL;
    foreach(ConnectorItem * ci, breadboardList) {
        QPointF ciPos = ci->sceneAdjustedTerminalPoint(NULL);
        double dist = QLineF(ciPos, pos).length();
        QString ciId = ci->connectorSharedID();
        bool breadboardUPDOWN = false;
        QRegularExpression re(QString("[W-Z]$"));
        QRegularExpressionMatch match = re.match(ciId);
        if (match.hasMatch()) {
            breadboardUPDOWN = true;
        }
        if (dist < minDist && ci->attachedTo() == m_breadBoardModelSet->keyItem() && breadboardUPDOWN) {
            minDist = dist;
            minC = ci;
        }
    }

    if (minC) return minC->connectorSharedID();
    return "";
}

void SketchWidget::completeSuggestion(QSharedPointer<ModelSet> modelset, bool transparent) {
    QList<QPair<ModelSet::Terminal, QString>> vccT = modelset->getPinTypeTerminal("VCC");
    QList<QPair<ModelSet::Terminal, QString>> gndT = modelset->getPinTypeTerminal("GND");
    QSharedPointer<SetConnection> setconnection;
    //if m_addedDefaultPart != NLL
    if (modelset->isMicrocontroller()) {
        if ((modelset->breadboardConnection()).isNull()) {
            setconnection = QSharedPointer<SetConnection>(new SetConnection(m_breadBoardModelSet, modelset));

            QList<QString> vccConnectorIDList({"pin3W", "pin3Y"});
            QList<QString> gndConnectorIDList({"pin3X", "pin3Z"});
            QString vccConnectorID = findBreadBoardUnused(vccConnectorIDList, QList<QString>(), true);
            QString gndConnectorID = findBreadBoardUnused(gndConnectorIDList, QList<QString>(), true);
            if (vccT.length() > 0 && vccConnectorID!="") {
                TerminalStringPair vccPair = vccT[0];
                foreach(TerminalStringPair pair, vccT) {
                    if (pair.second == "5V") {
                        vccPair = pair;
                        break;
                    }
                }
                //setconnection->appendConnection(vccPair.first.connectorID, vccConnectorID, QColor(255, 0, 0));
                setconnection->appendConnection(vccConnectorID, vccPair.first.name, QColor(255, 0, 0));
                m_breadBoardModelSet->insertTerminalType(vccConnectorID, "VCC");
                m_breadBoardModelSet->insertTerminalType(vccConnectorID, vccPair.second);
            }
            if (gndT.length() > 0 && gndConnectorID!="") {
                setconnection->appendConnection(gndConnectorID, gndT[0].first.name,  QColor(0, 0, 0));
                //setconnection->appendConnection(gndT[0].first.connectorID, gndConnectorID, QColor(0, 0, 0));
                m_breadBoardModelSet->insertTerminalType(gndConnectorID, gndT[0].second);
                if (m_breadBoardModelSet->breadboardConnection().isNull()) {
                    QSharedPointer<SetConnection> breadBoardSetconnection = QSharedPointer<SetConnection>(new SetConnection(m_breadBoardModelSet, m_breadBoardModelSet));
                    breadBoardSetconnection->appendConnection("pin3X", "pin3Z");
                    m_breadBoardModelSet->addBreadboardConnection(breadBoardSetconnection);
                }
            }
            modelset->addBreadboardConnection(setconnection);
        }

    } else {
        setconnection = QSharedPointer<SetConnection>(new SetConnection(m_breadBoardModelSet, modelset));
        foreach (TerminalStringPair pair, vccT) {
            QList<QPair<ModelSet::Terminal, QString>> breadboardT = m_breadBoardModelSet->getPinTypeTerminal(pair.second);
            QList<QString> vccConnectorIDList;
            if (breadboardT.length() > 0) {
                ModelSet::Terminal t = breadboardT[0].first;
                vccConnectorIDList.append(t.connectorID);
            } else {
                vccConnectorIDList.append("pin3W");
                vccConnectorIDList.append("pin3Y");
            }
            //TODO: findBreadBoardNearest
            ConnectorItem * ci = modelset->keyItem()->findConnectorItemWithSharedID(pair.first.connectorID);
            QString vccConnectorID = vccConnectorIDList[0];
            //if (ci) vccConnectorID = findBreadBoardNearest(ci->sceneAdjustedTerminalPoint(NULL), vccConnectorIDList, vccConnectorIDList, false);
            //else vccConnectorID = findBreadBoardUnused(vccConnectorIDList, vccConnectorIDList, false);
            //ItemBase * wire = addSetWire(modelset->getItem(pair.first.label), pair.first.connectorID, m_breadBoardModelSet->keyItem(), vccConnectorID, transparent);
            setconnection->appendConnection(vccConnectorID, pair.first.name, QColor(255, 0, 0));
            m_breadBoardModelSet->insertTerminalType(vccConnectorID, "VCC");
            m_breadBoardModelSet->insertTerminalType(vccConnectorID, pair.second);

//            ModelSet::Terminal t = modelset->getConnectedTerminal(pair.first);
//            if (t.label != "") {
//                setconnection->appendConnection(vccConnectorID, t.name, QColor(255, 0, 0));
//                //ItemBase * wire = addSetWire(modelset->getItem(pair.first.label), pair.first.connectorID, m_breadBoardModelSet->keyItem(), vccConnectorID, transparent);
//            }
            //pair.first.
        }

        foreach (TerminalStringPair pair, gndT) {
            QList<QPair<ModelSet::Terminal, QString>> breadboardT = m_breadBoardModelSet->getPinTypeTerminal(pair.second);
            QList<QString> gndConnectorIDList;
            if (breadboardT.length() > 0) {
                ModelSet::Terminal t = breadboardT[0].first;
                gndConnectorIDList.append(t.connectorID);
            } else {
                gndConnectorIDList.append("pin3X");
                gndConnectorIDList.append("pin3Z");
            }
            //TODO: findBreadBoardNearest
            ConnectorItem * ci = modelset->keyItem()->findConnectorItemWithSharedID(pair.first.connectorID);
            QString gndConnectorID;
            if (ci) gndConnectorID = findBreadBoardNearest(ci->sceneAdjustedTerminalPoint(NULL), gndConnectorIDList, gndConnectorIDList, false);
            else gndConnectorID = findBreadBoardUnused(gndConnectorIDList, gndConnectorIDList, false);
            //ItemBase * wire = addSetWire(modelset->getItem(pair.first.label), pair.first.connectorID, m_breadBoardModelSet->keyItem(), gndConnectorID, transparent);
            setconnection->appendConnection(gndConnectorID, pair.first.name, QColor(0, 0, 0));
            m_breadBoardModelSet->insertTerminalType(gndConnectorID, pair.second);
        }
        if (vccT.length() + gndT.length() > 0) modelset->addBreadboardConnection(setconnection);

    }
//    if (!setconnection.isNull()) {
//        addSetConnection(setconnection, transparent);
//        modelset->addSetConnection(setconnection);
//        if (!transparent) setconnection->setConfirm();
//    }

}


void SketchWidget::checkMousePressSuggestion(QGraphicsItem * item) {

    if (!m_prevModelSet.isNull()) findKeyItem(m_prevModelSet);
    bool isModelSet = false;
    ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
    if (itemBase) {
        QSharedPointer<ModelSet> modelset = itemBase->modelSet();
        if (!modelset.isNull()) {
            m_pressModelSet = modelset;
            isModelSet = true;
        }
    }

    if (!isModelSet) {
        m_pressItem = itemBase;
        removePrevModelSet();
        AutoCompleter::clearRecommend();
    }
    return;
}

void SketchWidget::checkSelectSuggestion() {


    if (m_pressItem) {

        AutoCompleter::getSuggestionSet(m_pressItem, this);
        m_pressItem = NULL;
        return;
    }

    if (!m_autoComplete || m_pressModelSet.isNull()) return;
    if (m_pressModelSet != m_prevModelSet) {
        if (!m_prevModelSet.isNull()) {
            QSharedPointer<SetConnection> prevSetConnection = m_prevModelSet->setConnection();
            if (m_prevModelSet->isConfirm() && !prevSetConnection.isNull()) {
                if (prevSetConnection->getFromModelSet() == m_pressModelSet) {
                    newAddSetConnectionCommand(prevSetConnection, NULL);
                }
            }
        }
        removePrevModelSet();
    }
    QSharedPointer<SetConnection> setConnection = m_pressModelSet->setConnection();
    if (!m_pressModelSet->isConfirm()) {
        //confirm!
        if (setConnection.isNull()) {
            //complete model set only
            selectModelSet(m_pressModelSet, false, false);
        } else {
            //complete module to module
            selectSetToSet(m_pressModelSet, setConnection, false, false);
        }
    } else {
        if (!setConnection.isNull()) {
            if (setConnection->isConfirm()) {
                // suggest next
                AutoCompleter::getSuggestionNext(m_pressModelSet, this);
            } else {
                // A-B wiring confirm
                newAddSetConnectionCommand(setConnection, NULL);
            }
        } else {
            AutoCompleter::getSuggestionNext(m_pressModelSet, this);
        }
    }

//    //TODO: SET to SET
//    if (m_pressModelSet->setConnection().isNull()) {
//        selectModelSet(m_pressModelSet, false);
//    } else {
//        setOpacity(m_pressModelSet);
//        confirmSelect(m_pressModelSet);
//    }

//    //setOpacity(m_pressModelSet);
//    //confirmSelect(m_pressModelSet);
}

void SketchWidget::confirmSelect(QSharedPointer<ModelSet> modelSet) {

//    modelSet->setConfirm();
//    if (!modelSet->setConnection().isNull())
//        modelSet->setConnection()->setConfirm();
//    if (!modelSet->breadboardConnection().isNull()) {
//        modelSet->breadboardConnection()->setConfirm();
//    }
    m_prevModelSet.reset();
    m_pressModelSet.reset();
    /*
    if (modelSet->keyId() != -1) {
        ItemBase * keyItem = findItem(modelSet->keyId());
        if (keyItem != NULL) modelSet->setKeyItem(keyItem);
    }*/
    //check for exist 
    //AutoCompleter::getSuggestionExist(modelSet, this);
    AutoCompleter::getSuggestionNext(modelSet, this);
    //m_savedModelSet.append(modelSet);
    return;
}

void SketchWidget::setOpacity(QSharedPointer<ModelSet> modelSet) {
    if (modelSet.isNull()) return;
    if (!modelSet->isConfirm()) {
        modelSet->setConfirm();

        QList<ItemBase *> itemList = modelSet->getItemList();
        //if (itemList.length() == 0) return;
        foreach(ItemBase * item, itemList) {
            if (item->opacity() == 1) continue;
            item->setOpacity(1);
            QPair<ModelSet::Terminal, ModelSet::Terminal> tpair = modelSet->getWireConnection(item);
            ModelSet::Terminal from = tpair.first;
            ModelSet::Terminal to = tpair.second;
            if (from.label != "") {
                ItemBase * fromItem = modelSet->getItem(from.label);
                changeConnection(fromItem->id(), from.connectorID, item->id(), "connector0", ViewLayer::specFromID(fromItem->findConnectorItemWithSharedID(from.connectorID)->attachedToViewLayerID()), true, false, false);
            }
            if (to.label != "") {
                ItemBase * toItem = modelSet->getItem(to.label);
                changeConnection(toItem->id(), to.connectorID, item->id(), "connector1", ViewLayer::specFromID(toItem->findConnectorItemWithSharedID(to.connectorID)->attachedToViewLayerID()), true, false, false);
            }
        }
    }
    //setOpacity(modelSet->setConnection());
    //setOpacity(modelSet->breadboardConnection());
}

void SketchWidget::setOpacity(QSharedPointer<SetConnection> setConnection) {
    if (setConnection.isNull()) return;
    if (setConnection->isConfirm()) return;
    
    setConnection->setConfirm();
    QSharedPointer<ModelSet> fromModelSet = setConnection->getFromModelSet();
    QSharedPointer<ModelSet> toModelSet = setConnection->getToModelSet();
    fromModelSet->appendSetConnectionList(0, setConnection);
    toModelSet->appendSetConnectionList(1, setConnection);

    QList<ItemBase *> itemList = setConnection->getWireList();
    //if (itemList.length() == 0) return;
    foreach(ItemBase * item, itemList) {
        if (item->opacity() == 1) continue;
        item->setOpacity(1);
        QPair<ModelSet::Terminal, ModelSet::Terminal> tpair = setConnection->getWireConnection(item);
        ModelSet::Terminal from = tpair.first;
        ModelSet::Terminal to = tpair.second;
        if (from.label != "") {
            ItemBase * fromItem = fromModelSet->getItem(from.label);
            changeConnection(fromItem->id(), from.connectorID, item->id(), "connector0", ViewLayer::specFromID(fromItem->findConnectorItemWithSharedID(from.connectorID)->attachedToViewLayerID()), true, false, false);
        }
        if (to.label != "") {
            ItemBase * toItem = toModelSet->getItem(to.label);
            changeConnection(toItem->id(), to.connectorID, item->id(), "connector1",ViewLayer::specFromID(toItem->findConnectorItemWithSharedID(to.connectorID)->attachedToViewLayerID()), true, false, false);
        }
    }
    
}

QList<QSharedPointer<ModelSet>> SketchWidget::getSavedModelSets() {
    return m_savedModelSet;
}


bool SketchWidget::dragEnterEventFromRecommend(QDragEnterEvent * event) {
    //DebugDialog::debug("list drag----------------------------------------------5");
    if (!event->mimeData()->hasFormat("type/setToSet")) return false;
    m_draggingSuggestion = true;
    m_dragModelSet = m_prevModelSet;
    //m_tempPoint = event->pos();
    //updateModelSetPos(event->pos());

//    QByteArray itemData = event->mimeData()->data("type/setToSet");
//    QDataStream dataStream(&itemData, QIODevice::ReadOnly);
//    //QSharedPointer<ModelSet> toModelset ;
//    //QSharedPointer<SetConnection> setConnection ;
//    DebugDialog::debug("list drag----------------------------------------------5");
//    //QVariantList itemDataV ;
//    int rowNumber ;
//    dataStream >> rowNumber ;
//    //dataStream >> toModelset >> setConnection;
//    DebugDialog::debug("list drag----------------------------------------------6");
//    //addSetToSet(m_toModelsetList[rowNumber],m_setConnectionList[rowNumber],false);
//    DebugDialog::debug("list drag----------------------------------------------7");

    return true;

}

bool SketchWidget::dragMoveEventFromRecommend(QDragMoveEvent * event) {

    if (!event->mimeData()->hasFormat("type/setToSet")) return false;
    DebugDialog::debug("in?");
    updateModelSetPos(event->pos());
    return true;
}

bool SketchWidget::dropEventFromRecommend(QDropEvent * event) {
    DebugDialog::debug("dropEnter");
    if (!event->mimeData()->hasFormat("type/setToSet")) return false;
    //selectSetToSet(m_dragModelSet,  m_dragModelSet->setConnection(), false, false);
    selectSetToSet(m_prevModelSet,  m_prevModelSet->setConnection(), false, false);
    return true;
}

void SketchWidget::updateModelSetPos(QPoint pos) {

    if (m_dragModelSet.isNull()) return;
    QPointF mousePos = this->mapToScene(pos);

    bool useOrigin = m_prevModelSet == m_dragModelSet ? false : true;
    QSharedPointer<ModelSet> originModelSet;

    foreach(QSharedPointer<ModelSet> modelset, m_savedModelSet) {
        ItemBase * keyItem = modelset->keyItem();
       QRectF itemRect = keyItem->boundingRect();
        QPointF topLeft = keyItem->pos();
        if (itemRect.contains(mousePos-topLeft) && m_dragModelSet->setId() == modelset->setId()) {
            if (m_prevModelSet == modelset) break;
            useOrigin = true;
            originModelSet = modelset;
            QSharedPointer<SetConnection> setConnection = m_dragModelSet->setConnection()->clone();
            setConnection->setModelSet(1, originModelSet);
            selectSetToSet(originModelSet, setConnection, false, true);
            originModelSet->addSetConnection(setConnection);
            //removePrevModelSet();
            //m_dragModelSet = modelset;
            break;
        } else {
            useOrigin = false;
        }

    }

    if (useOrigin) return;

    if (m_prevModelSet != m_dragModelSet) {

        QSharedPointer<SetConnection> setConnection = m_prevModelSet->setConnection()->clone();
        setConnection->setModelSet(1, m_dragModelSet);
        selectSetToSet(m_dragModelSet, setConnection, false, true);
        m_dragModelSet->addSetConnection(setConnection);

    }
    ItemBase * item = m_dragModelSet->keyItem();
    if (!item) return;
    QPointF oriPos = item->pos();
    QPointF offset = mousePos - oriPos;

    QList<ItemBase *> itemList = m_dragModelSet->getItemList();
    //if (itemList.length() == 0) return;
    foreach(ItemBase * item, itemList) {
        item->setPos(this->mapToScene(pos));
        //item->setPos(oriPos);
    }

    QSharedPointer<SetConnection> breadBoardConnection = m_dragModelSet->breadboardConnection();
    if (breadBoardConnection) {
        itemList = breadBoardConnection->getWireList();
        //if (itemList.length() == 0) return;
        foreach(ItemBase * item, itemList) {
            Wire * wire = qobject_cast<Wire *>(item);
            if (wire) {
                QLineF oriLine = wire->line();
                QLineF newLine(oriLine.x1(), oriLine.y1(), oriLine.x2()+offset.x(), oriLine.y2()+offset.y());
                wire->setLine(newLine);
            }
        }
    }

    //setModelSet

    QSharedPointer<SetConnection> setConnection = m_dragModelSet->setConnection();
    if (setConnection == NULL) return;

    QList<ItemBase *> wireList = setConnection->getWireList();
    //if (itemList.length() == 0) return;
    foreach(ItemBase * item, wireList) {
        Wire * wire = qobject_cast<Wire *>(item);
        if (wire) {
            QPair<ModelSet::Terminal, ModelSet::Terminal> tpair = setConnection->getWireConnection(item);
            ModelSet::Terminal from = tpair.first;
            ModelSet::Terminal to = tpair.second;
            if (from.label != "" && to.label != "") {
                QLineF oriLine = wire->line();
                QLineF newLine;
                if (!useOrigin) {
                    newLine = QLineF(oriLine.x1(), oriLine.y1(), oriLine.x2()+offset.x(), oriLine.y2()+offset.y());
                } else {
                    ItemBase * fromItem = setConnection->getFromModelSet()->getItem(from.label);
                    ItemBase *  toItem = originModelSet->getItem(to.label);
                    ConnectorItem * fromConnectorItem = fromItem->findConnectorItemWithSharedID(from.connectorID);
                    ConnectorItem * toConnectorItem = toItem->findConnectorItemWithSharedID(to.connectorID);
                    QPointF fromConnectorPos = fromConnectorItem->sceneAdjustedTerminalPoint(NULL);
                    QPointF toConnectorPos = toConnectorItem->sceneAdjustedTerminalPoint(NULL);

                    newLine = QLineF(oriLine.x1(), oriLine.y1(), -fromConnectorPos.x()+toConnectorPos.x(), -fromConnectorPos.y()+toConnectorPos.y());
                }

                wire->setLine(newLine);
            }

        }
    }
}

