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
    // 组件数据变更的**统一入口**（编辑对话框确认后调用）：
    // 更新数据 + 让图片缓存按新路径失效重载 + 重绘。
    // 任何会改动图片路径的编辑都必须走这里，否则画布会继续显示旧图。
    void applyComponentData(const Component& rComponent);

    // 项目目录：用于解析卡片边框图片的相对路径（由 CanvasScene 注入）
    void setProjectDirectory(const QString& strDir);

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
    void editStickerContent();
    // 卡片边框：弹专用对话框改形状（矩形/正方形/圆形/椭圆）、颜色与框内图片
    void editCardBorder();
    // 组件用到的图片路径（图片组件取 imageData，卡片边框取框内图片；已解析为绝对路径）
    QString componentImagePath(const Component& rComponent) const;
    // 图片路径变化时重新加载缓存
    void refreshImageCache();
    qreal handleHitRadius() const;

    Component m_component;                    // 组件数据副本
    E_HANDLE_TYPE m_eActiveHandle = E_HANDLE_NONE;   // 当前激活的命中区域
    QPointF m_pressPos;                       // 按下时组件位置
    QSizeF m_pressSize;                       // 按下时组件尺寸
    QPointF m_pressMouseLocal;                // 按下时鼠标在组件内的局部坐标
    qreal m_dRotateStartAngle = 0;            // 旋转起始角
    QPointF m_pressMouseScene;                // 按下时鼠标场景坐标
    QImage m_imageCache;                      // 图片缓存（图片组件与卡片边框图片共用）
    QString m_strCachedImagePath;             // 缓存对应的图片路径（变化时重载）
    QString m_strProjectDirectory;            // 项目目录（解析卡片边框图片相对路径）
    // 多选拖拽：其余选中组件的起始位置（仅拖动按下组件时联动）
    QVector<ComponentItem*> m_vecDragItems;
    QVector<QPointF> m_vecDragStartPos;
};

} // namespace bwm

#endif // BWM_EDITOR_COMPONENTITEM_H
