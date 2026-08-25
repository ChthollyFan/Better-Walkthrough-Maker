/**
 * @file CanvasScene.h
 * @author zhangweimu
 * @brief 画布场景：管理页面组件图元、选择集与模型同步（M2a）。
 */
#ifndef BWM_EDITOR_CANVASSCENE_H
#define BWM_EDITOR_CANVASSCENE_H

#include <QGraphicsScene>

#include "core/Component.h"
#include "core/Project.h"

namespace bwm {

class ComponentItem;

// 画布场景：负责把页面组件数据渲染为图元，并把图元状态同步回模型。
class CanvasScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit CanvasScene(QObject* pParent = nullptr);

    // 用页面数据重建场景（清空后按 zOrder 创建全部组件图元）
    void loadPage(const Page& rPage);
    // 将场景内全部组件写回页面模型（按 zOrder 排序）
    void syncToModel(Page* pPage);
    // 新增一个组件（自动分配 id 与 zOrder），返回创建的图元
    ComponentItem* addComponent(const Component& rComponent);
    // 删除当前选中组件
    void removeSelectedComponents();
    // 选中组件列表（按 zOrder 排序）
    QVector<ComponentItem*> selectedComponentItems() const;
    // 全部组件图元（按 zOrder 排序）
    QVector<ComponentItem*> componentItems() const { return m_vecItems; }
    // 按 zOrder 重排图元并刷新场景 Z 值（图层交换后调用）
    void sortByZOrder();

signals:
    // 组件增删或几何/内容变化后发出（供主窗口同步模型与脏标记）
    void componentsChanged();

private:
    void rebuildItems(const QVector<Component>& rComponents);

    QVector<ComponentItem*> m_vecItems;   // 全部组件图元（按 zOrder 排序）
};

} // namespace bwm

#endif // BWM_EDITOR_CANVASSCENE_H
