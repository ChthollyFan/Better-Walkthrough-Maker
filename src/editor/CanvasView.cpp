/**
 * @file CanvasView.cpp
 * @author zhangweimu
 * @brief 画布视图实现：滚轮缩放与中键平移。
 */
#include "editor/CanvasView.h"

#include <QScrollBar>
#include <QWheelEvent>

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

} // namespace bwm
