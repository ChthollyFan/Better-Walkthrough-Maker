/**
 * @file MainWindow.h
 * @author zhangweimu
 * @brief 主窗口：菜单栏、工具栏、项目树、画布、图层面板、状态栏。
 *
 * 重构后 MainWindow 仅作为组装者与协调者：
 * - 创建 PluginHost 并注册内置插件
 * - 创建各面板（ProjectTreePanel / AssetPanel / LayerPanel）并连接信号
 * - 构建菜单栏与工具栏（从 PluginHost 动态获取组件类型列表）
 * - 路由跨模块信号（如项目树→画布刷新、图层→模型同步）
 * - 处理文件操作（新建/打开/保存/关闭）与窗口标题
 *
 * 不再包含各面板/对话框的内部逻辑，这些已迁移到独立模块。
 */
#ifndef BWM_APP_MAINWINDOW_H
#define BWM_APP_MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

#include "core/Component.h"
#include "plugin/PluginContext.h"
#include "plugin/IComponentProvider.h"

class QMenu;
class QTabWidget;
class QToolBar;
class QUndoStack;

namespace bwm {

class CanvasScene;
class CanvasView;
class ComponentItem;
class LayerPanel;
class PluginHost;
class ProjectManager;
class ProjectTreePanel;
class AssetPanel;

// 主窗口：组装各面板与画布，路由跨模块信号。
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* pParent = nullptr);
    ~MainWindow() override;

protected:
    // 关闭前检查未保存更改（保存/不保存/取消）
    void closeEvent(QCloseEvent* pEvent) override;

private slots:
    // 文件操作
    void onNewProject();
    void onOpenProject();
    void onOpenRecentProject();
    void onSaveProject();
    void onExportPng();
    void onCopyPageToClipboard();
    void onShowSettings();
    void onShowShortcuts();
    void onShowAbout();
    void onToggleAutoSave(bool bEnabled);
    void onProjectOpened();
    void onAutoSavePerformed(bool bOk, const QString& strMessage);

    // 项目树信号处理
    void onPageSelected(const QString& rPageKey);
    void onProjectStructureChanged();

    // 画布与模型同步
    void onCanvasComponentsChanged();
    void onCanvasSelectionChanged();
    void onComponentEditStarted();
    void onComponentEditFinished();

    // 编辑操作
    void onDeleteSelected();
    void onSelectAllComponents();
    void onCopy();
    void onPaste();
    void onCut();
    void onAlignComponents(int nAlign);
    void onDistributeComponents(bool bHorizontal);
    void onCanvasContextMenu(const QPointF& rScenePos);

    // 吸附开关
    void onToggleSnapToGrid(bool bEnable);
    void onToggleSnapToGuides(bool bEnable);

    // 素材面板信号处理
    void onAssetInserted(const Component& rComponent);

private:
    // 对齐类型常量（onAlignComponents 的参数）
    enum E_ALIGN_TYPE {
        E_ALIGN_LEFT = 0,
        E_ALIGN_H_CENTER,
        E_ALIGN_RIGHT,
        E_ALIGN_TOP,
        E_ALIGN_V_CENTER,
        E_ALIGN_BOTTOM,
    };

    // ---- UI 构建 ----
    void createMenus();
    void createToolBar();
    void createCentralWidget();
    void createStatusBar();
    void refreshRecentProjectsMenu();

    // ---- 画布与模型同步 ----
    void updateCanvasEditor();
    void syncCanvasToModel();
    void applyTheme();

    // ---- 项目/页面辅助 ----
    void openProjectPath(const QString& strJsonPath);
    Page* currentPage();
    QVector<Component> currentComponents();
    void pushSnapshot(const QString& strText, const QVector<Component>& rBefore,
                      const QVector<Component>& rAfter);
    void updateWindowTitle();

    // ---- 组件插入（统一入口，供菜单/工具栏调用）----
    void insertComponent(const IComponentProvider* pProvider);

    // ---- 图层辅助 ----
    void setItemToTop(ComponentItem* pItem, bool bTop);
    ComponentItem* componentItemAt(const QPointF& rScenePos) const;

    // ---- 构建当前插件上下文 ----
    PluginContext makeContext() const;

    // ---- 核心对象 ----
    PluginHost* m_pHost = nullptr;                ///< 插件宿主
    ProjectManager* m_pProjectManager = nullptr;  ///< 项目管理
    CanvasScene* m_pScene = nullptr;              ///< 画布场景
    CanvasView* m_pView = nullptr;                ///< 画布视图
    QUndoStack* m_pUndoStack = nullptr;           ///< 撤销栈

    // ---- 面板 ----
    ProjectTreePanel* m_pTreePanel = nullptr;     ///< 项目树面板
    AssetPanel* m_pAssetPanel = nullptr;          ///< 素材库面板
    LayerPanel* m_pLayerPanel = nullptr;          ///< 图层面板
    QTabWidget* m_pTabPanel = nullptr;            ///< 右侧标签页容器

    // ---- UI 控件 ----
    QMenu* m_pRecentProjectsMenu = nullptr;       ///< 最近项目菜单
    QToolBar* m_pToolBar = nullptr;               ///< 工具栏

    // ---- 状态标志 ----
    bool m_bSyncingCanvas = false;                ///< 防止同步时信号回环
    bool m_bInEditTransaction = false;            ///< 是否处于编辑事务中
    QVector<Component> m_editBeforeSnapshot;      ///< 编辑事务前的组件快照
    QVector<Component> m_vecClipboard;            ///< 应用内剪贴板
};

} // namespace bwm

#endif // BWM_APP_MAINWINDOW_H
