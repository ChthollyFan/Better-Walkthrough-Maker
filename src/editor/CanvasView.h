/**
 * @file CanvasView.h
 * @author zhangweimu
 * @brief 画布视图：滚轮缩放与中键平移（M2a）。
 */
#ifndef BWM_EDITOR_CANVASVIEW_H
#define BWM_EDITOR_CANVASVIEW_H

#include <QColor>
#include <QGraphicsView>

namespace bwm {

// 画布视图：Ctrl+滚轮缩放（以鼠标为中心）、中键拖拽平移、网格绘制。
class CanvasView : public QGraphicsView {
    Q_OBJECT
public:
    explicit CanvasView(QGraphicsScene* pScene, QWidget* pParent = nullptr);

    /**
     * @brief 画布视口的默认背景色。
     *
     * 无 UI 风格干预（如玻璃风格把视口设为全透明）时使用。
     * UI 风格切换时由 MainWindow 调用 setBackgroundBrush 覆盖或还原为本色。
     */
    static QColor defaultBackgroundColor();

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
