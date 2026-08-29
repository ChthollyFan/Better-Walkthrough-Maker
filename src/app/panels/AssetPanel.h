/**
 * @file AssetPanel.h
 * @author zhangweimu
 * @brief 素材库面板：项目 assets/ 目录下的图片素材缩略图列表。
 *
 * 从原 MainWindow 的素材库相关逻辑中拆分。面板持有 ProjectManager 指针
 * 读取项目目录，双击/右键插入图片组件到画布。
 *
 * 信号：
 * - assetInserted(Component)  用户双击/右键插入素材时发出，MainWindow 接收后添加到画布
 */
#ifndef BWM_APP_PANELS_ASSETPANEL_H
#define BWM_APP_PANELS_ASSETPANEL_H

#include <QWidget>

#include "core/Component.h"

class QListWidget;
class QListWidgetItem;

namespace bwm {

class ProjectManager;

/**
 * @brief 素材库面板。
 *
 * 功能：
 * - 显示项目 assets/ 目录下的图片素材缩略图
 * - 导入素材按钮（复制外部图片到 assets/）
 * - 双击/右键插入素材到画布
 * - 右键删除素材（含引用检查）
 */
class AssetPanel : public QWidget
{
    Q_OBJECT
public:
    /**
     * @param pParent           父 widget
     * @param pProjectManager   项目管理器（读取项目目录与引用检查）
     */
    AssetPanel(QWidget* pParent, ProjectManager* pProjectManager);

    /**
     * @brief 刷新素材列表（项目打开或导入素材后调用）。
     */
    void refreshAssetList();

signals:
    /**
     * @brief 用户请求插入素材到画布。
     * @param rComponent  已填充 imagePath 的图片组件
     * MainWindow 接收后调用 CanvasScene::addComponent 添加到画布。
     */
    void assetInserted(const Component& rComponent);

    /**
     * @brief 素材列表已变更（导入/删除后），通知 MainWindow 刷新状态栏等。
     */
    void assetsChanged();

private slots:
    void onImportAssets();
    void onAssetDoubleClicked(QListWidgetItem* pItem);
    void onAssetContextMenu(const QPoint& rPos);

private:
    ProjectManager* m_pProjectManager;   ///< 项目管理器
    QListWidget* m_pList;                ///< 素材缩略图列表
};

} // namespace bwm

#endif // BWM_APP_PANELS_ASSETPANEL_H
