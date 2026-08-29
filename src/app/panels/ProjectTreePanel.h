/**
 * @file ProjectTreePanel.h
 * @author zhangweimu
 * @brief 项目树面板：显示项目结构（项目→攻略→页面），支持增删改与右键菜单。
 *
 * 从原 MainWindow 的项目树相关逻辑中拆分。面板持有 ProjectManager 指针
 * 直接操作项目数据，完成后通过信号通知 MainWindow 刷新画布和标题。
 *
 * 节点键格式：
 * - ""         → 项目根节点
 * - "W"        → 攻略节点（W 为攻略索引）
 * - "W:P"      → 页面节点（W 为攻略索引，P 为页面索引）
 */
#ifndef BWM_APP_PANELS_PROJECTTREEPANEL_H
#define BWM_APP_PANELS_PROJECTTREEPANEL_H

#include <QHash>
#include <QWidget>

#include "core/Project.h"

class QTreeWidget;
class QTreeWidgetItem;

namespace bwm {

class ProjectManager;
class PluginHost;

/**
 * @brief 项目树面板。
 *
 * 显示项目的树形结构，支持：
 * - 选中页面节点 → 发出 pageSelected 信号通知 MainWindow 加载画布
 * - 右键菜单 → 新建/重命名/删除攻略与页面
 * - 模板操作 → 保存为模板/导入模板/导出模板
 * - 结构变更 → 发出 projectStructureChanged 信号通知 MainWindow 刷新
 */
class ProjectTreePanel : public QWidget
{
    Q_OBJECT
public:
    /**
     * @param pParent           父 widget
     * @param pProjectManager   项目管理器（面板直接操作其数据）
     * @param pHost             插件宿主（获取模板 Provider 列表）
     */
    ProjectTreePanel(QWidget* pParent, ProjectManager* pProjectManager, PluginHost* pHost);

    /**
     * @brief 重建项目树（项目打开或结构变更后调用）。
     */
    void rebuildProjectTree();

    /**
     * @brief 获取当前选中页面节点的键（"W:P" 格式），非页面节点返回空。
     */
    QString selectedPageKey() const;

    /**
     * @brief 获取当前选中节点的键（""/"W"/"W:P"）。
     */
    QString selectedNodeKey() const;

    /**
     * @brief 按键选中节点（用于打开项目后自动选中首页）。
     */
    void selectNodeByKey(const QString& rKey);

signals:
    /**
     * @brief 选中了页面节点（或切换到非页面节点）。
     * @param rPageKey  页面键（"W:P"），空表示未选中页面。
     */
    void pageSelected(const QString& rPageKey);

    /**
     * @brief 项目结构变更（增删攻略/页面/重命名）。
     * MainWindow 接收后刷新画布、标题等。
     */
    void projectStructureChanged();

    /**
     * @brief 请求刷新素材列表（新建攻略后素材库可能需要更新）。
     */
    void assetsChanged();

public slots:
    // 树选择变化
    void onSelectionChanged();
    // 右键菜单
    void onContextMenu(const QPoint& rPos);
    // 攻略/页面管理
    void onAddWalkthrough();
    void onAddPage();
    void onRenameNode();
    void onDeleteNode();
    // 模板操作
    void onSaveAsTemplate();
    void onImportTemplate();
    void onExportTemplate();

private:
    ProjectManager* m_pProjectManager;   ///< 项目管理器
    PluginHost* m_pHost;                 ///< 插件宿主
    QTreeWidget* m_pTree;                ///< 项目树控件

    ///< 树节点 → 数据路径键（"W"/"W:P"/""）
    QHash<QTreeWidgetItem*, QString> m_mapNodeKeys;
};

} // namespace bwm

#endif // BWM_APP_PANELS_PROJECTTREEPANEL_H
