/**
 * @file CanvasView.h
 * @author zhangweimu
 * @brief 画布视图：滚轮缩放与中键平移（M2a）。
 */
#ifndef BWM_EDITOR_CANVASVIEW_H
#define BWM_EDITOR_CANVASVIEW_H

#include <QGraphicsView>

namespace bwm {

// 画布视图：Ctrl+滚轮缩放（以鼠标为中心）、中键拖拽平移、网格绘制。
class CanvasView : public QGraphicsView {
    Q_OBJECT
public:
    explicit CanvasView(QGraphicsScene* pScene, QWidget* pParent = nullptr);

signals:
    // 右键菜单请求（场景坐标）
    void contextMenuRequested(const QPointF& rScenePos);

protected:
    void wheelEvent(QWheelEvent* pEvent) override;
    void drawBackground(QPainter* pPainter, const QRectF& rRect) override;
    void contextMenuEvent(QContextMenuEvent* pEvent) override;
};

} // namespace bwm

#endif // BWM_EDITOR_CANVASVIEW_H
