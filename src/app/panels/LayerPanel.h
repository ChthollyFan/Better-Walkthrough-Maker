/**
 * @file LayerPanel.h
 * @author zhangweimu
 * @brief 图层面板：显示画布组件列表，支持选中/移动/可见性/置顶置底。
 *
 * 从原 MainWindow 的图层相关逻辑中拆分。面板持有 CanvasScene 指针
 * 直接操作组件图元的 zOrder 和可见性，完成后通过信号通知 MainWindow
 * 同步模型。
 *
 * 列表行 0 在最底层（zOrder 最小），最后一行在最顶层。
 */
#ifndef BWM_APP_PANELS_LAYERPANEL_H
#define BWM_APP_PANELS_LAYERPANEL_H

#include <QWidget>

class QListWidget;
class QListWidgetItem;

namespace bwm {

class CanvasScene;
class CanvasView;
class ComponentItem;

/**
 * @brief 图层面板。
 *
 * 功能：
 * - 显示当前页面全部组件（按 zOrder 排序，底层在上方）
 * - 点击行 → 选中对应画布图元
 * - 勾选/取消勾选 → 切换组件可见性
 * - 上移/下移/置顶/置底 → 调整 zOrder
 *
 * m_bSyncingCanvas 标志用于防止场景↔面板双向同步时的信号回环。
 */
class LayerPanel : public QWidget
{
    Q_OBJECT
public:
    /**
     * @param pParent  父 widget
     * @param pScene   画布场景（面板直接操作组件 zOrder）
     * @param pView    画布视图（选中图层后 centerOn）
     */
    LayerPanel(QWidget* pParent, CanvasScene* pScene, CanvasView* pView);

    /**
     * @brief 刷新图层列表（页面切换或组件变更后调用）。
     */
    void refreshLayerList();

    /**
     * @brief 场景选中变化时同步面板选中行。
     * 由 MainWindow 在 CanvasScene::selectionChanged 时调用。
     */
    void syncSelectionFromScene();

    /**
     * @brief 获取/设置同步标志（防止信号回环）。
     */
    bool isSyncing() const { return m_bSyncing; }

signals:
    /**
     * @brief 图层顺序或可见性已变更。
     * MainWindow 接收后调用 syncCanvasToModel 落盘。
     */
    void layerChanged();

private slots:
    void onMoveUp();
    void onMoveDown();
    void onToTop();
    void onToBottom();
    void onSelectionChanged();
    void onVisibilityChanged(QListWidgetItem* pListItem);

private:
    /**
     * @brief 按偏移量移动图层（+1 上移，-1 下移）。
     */
    void moveLayer(int nOffset);

    /**
     * @brief 移动图层到指定行索引。
     */
    void moveLayerTo(int nTargetIndex);

    CanvasScene* m_pScene;           ///< 画布场景
    CanvasView* m_pView;             ///< 画布视图
    QListWidget* m_pList;            ///< 图层列表
    bool m_bSyncing = false;         ///< 同步标志（防回环）
};

} // namespace bwm

#endif // BWM_APP_PANELS_LAYERPANEL_H
