/**
 * @file ComponentItem.h
 * @author zhangweimu
 * @brief 画布组件图元：渲染组件数据并处理选择/移动/缩放/旋转交互（M2a）。
 */
#ifndef BWM_EDITOR_COMPONENTITEM_H
#define BWM_EDITOR_COMPONENTITEM_H

#include <QGraphicsObject>

#include "core/Component.h"

namespace bwm {

// 画布组件图元：数据与渲染分离——持有 Component 副本，渲染结果与导出共用。
class ComponentItem : public QGraphicsObject {
    Q_OBJECT
public:
    // 命中区域类型（手柄）
    enum E_HANDLE_TYPE {
        E_HANDLE_NONE = 0,       // 组件内部（移动）
        E_HANDLE_TOP_LEFT,       // 左上角
        E_HANDLE_TOP,            // 上边
        E_HANDLE_TOP_RIGHT,      // 右上角
        E_HANDLE_RIGHT,          // 右边
        E_HANDLE_BOTTOM_RIGHT,   // 右下角
        E_HANDLE_BOTTOM,         // 下边
        E_HANDLE_BOTTOM_LEFT,    // 左下角
        E_HANDLE_LEFT,           // 左边
        E_HANDLE_ROTATE,         // 旋转手柄
    };

    explicit ComponentItem(const Component& rComponent, QGraphicsItem* pParent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;

    // 数据访问与同步
    Component component() const { return m_component; }
    void setComponent(const Component& rComponent);

    // 双击编辑（文本/表格组件编辑内容）
    void editContent();

signals:
    // 组件几何或内容变化后发出（场景据此通知主窗口同步模型）
    void geometryChanged();
    // 一次编辑事务开始（按下开始移动/缩放/旋转时发出，供撤销命令记录快照）
    void editStarted();
    // 一次编辑事务结束（释放鼠标时发出，供撤销命令提交）
    void editFinished();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* pEvent) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* pEvent) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* pEvent) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* pEvent) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* pEvent) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* pEvent) override;

private:
    void paintContent(QPainter* pPainter);
    void paintSelectionDecoration(QPainter* pPainter);
    E_HANDLE_TYPE hitTestHandle(const QPointF& rLocalPos) const;
    QRectF handleRect(E_HANDLE_TYPE eHandle) const;
    void updateCursorByHandle(E_HANDLE_TYPE eHandle);
    void resizeByHandle(E_HANDLE_TYPE eHandle, const QPointF& rDelta);
    void updateRotateByMouse(const QPointF& rScenePos);
    void editTextContent();
    void editTableContent();
    qreal handleHitRadius() const;

    Component m_component;                    // 组件数据副本
    E_HANDLE_TYPE m_eActiveHandle = E_HANDLE_NONE;   // 当前激活的命中区域
    QPointF m_pressPos;                       // 按下时组件位置
    QSizeF m_pressSize;                       // 按下时组件尺寸
    QPointF m_pressMouseLocal;                // 按下时鼠标在组件内的局部坐标
    qreal m_dRotateStartAngle = 0;            // 旋转起始角
    QPointF m_pressMouseScene;                // 按下时鼠标场景坐标
    QImage m_imageCache;                      // 图片组件缓存（避免重复加载）
    // 多选拖拽：其余选中组件的起始位置（仅拖动按下组件时联动）
    QVector<ComponentItem*> m_vecDragItems;
    QVector<QPointF> m_vecDragStartPos;
};

} // namespace bwm

#endif // BWM_EDITOR_COMPONENTITEM_H
