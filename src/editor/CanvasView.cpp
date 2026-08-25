/**
 * @file CanvasView.cpp
 * @author zhangweimu
 * @brief 画布视图实现：滚轮缩放与中键平移。
 */
#include "editor/CanvasView.h"

#include <QContextMenuEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

#include "editor/CanvasScene.h"

namespace bwm {

namespace {

constexpr qreal dZoomStep = 1.15;   // 缩放步进
constexpr qreal dMinZoom = 0.1;     // 最小缩放
constexpr qreal dMaxZoom = 8.0;     // 最大缩放

} // namespace

CanvasView::CanvasView(QGraphicsScene* pScene, QWidget* pParent)
    : QGraphicsView(pScene, pParent)
{
    setRenderHint(QPainter::Antialiasing);
    setBackgroundBrush(QColor(60, 60, 60));
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
}

void CanvasView::wheelEvent(QWheelEvent* pEvent)
{
    // 仅 Ctrl+滚轮 缩放，普通滚轮交给滚动条（画布较大时滚动浏览）
    if (pEvent->modifiers() & Qt::ControlModifier) {
        const qreal dFactor = pEvent->angleDelta().y() > 0 ? dZoomStep : 1.0 / dZoomStep;
        const qreal dNewScale = transform().m11() * dFactor;
        if (dNewScale < dMinZoom || dNewScale > dMaxZoom) {
            return;
        }
        // 以鼠标位置为中心缩放
        const QPointF anchorPos = mapToScene(pEvent->position().toPoint());
        scale(dFactor, dFactor);
        const QPointF anchorPosAfter = mapToScene(pEvent->position().toPoint());
        const QPointF delta = anchorPosAfter - anchorPos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + int(delta.x() * transform().m11()));
        verticalScrollBar()->setValue(verticalScrollBar()->value() + int(delta.y() * transform().m11()));
        pEvent->accept();
        return;
    }
    QGraphicsView::wheelEvent(pEvent);
}

void CanvasView::drawBackground(QPainter* pPainter, const QRectF& rRect)
{
    QGraphicsView::drawBackground(pPainter, rRect);

    // 网格绘制：仅当场景开启网格吸附且缩放足够大时显示，避免网格过密
    const auto* pCanvasScene = qobject_cast<const CanvasScene*>(scene());
    if (!pCanvasScene || !pCanvasScene->snapToGrid()) {
        return;
    }
    if (transform().m11() < 0.5) {
        return;
    }
    const int nGridSize = pCanvasScene->gridSize();
    pPainter->setPen(QPen(QColor(200, 200, 200, 80), 0));
    const qreal dLeft = std::floor(rRect.left() / nGridSize) * nGridSize;
    const qreal dTop = std::floor(rRect.top() / nGridSize) * nGridSize;
    for (qreal dX = dLeft; dX < rRect.right(); dX += nGridSize) {
        pPainter->drawLine(QPointF(dX, rRect.top()), QPointF(dX, rRect.bottom()));
    }
    for (qreal dY = dTop; dY < rRect.bottom(); dY += nGridSize) {
        pPainter->drawLine(QPointF(rRect.left(), dY), QPointF(rRect.right(), dY));
    }
}

void CanvasView::contextMenuEvent(QContextMenuEvent* pEvent)
{
    emit contextMenuRequested(mapToScene(pEvent->pos()));
    // 不调用基类：菜单由主窗口统一弹出
}

} // namespace bwm
