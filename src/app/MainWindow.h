/**
 * @file MainWindow.h
 * @author zhangweimu
 * @brief 主窗口：菜单栏、工具栏、项目树、画布、图层面板、状态栏（M2a 画布版）。
 */
#ifndef BWM_APP_MAINWINDOW_H
#define BWM_APP_MAINWINDOW_H

#include <QHash>
#include <QMainWindow>

class QGraphicsView;
class QListWidget;
class QListWidgetItem;
class QMenu;
class QToolBar;
class QTreeWidget;
class QTreeWidgetItem;

namespace bwm {

struct Page;
class CanvasScene;
class CanvasView;
class ComponentItem;
class ProjectManager;

// 主窗口：菜单栏、工具栏、项目树、画布、图层面板、状态栏。
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* pParent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onOpenRecentProject();
    void onSaveProject();
    void onToggleAutoSave(bool bEnabled);
    void onProjectTreeSelectionChanged();
    void onProjectOpened();
    void onAutoSavePerformed(bool bOk, const QString& strMessage);

    // 插入组件
    void onAddImageComponent();
    void onAddTextComponent();
    void onAddShapeComponent(int nIndex);
    // 编辑
    void onDeleteSelected();
    void onSelectAllComponents();
    // 画布与模型同步
    void onCanvasComponentsChanged();
    void onCanvasSelectionChanged();
    // 图层操作
    void onLayerMoveUp();
    void onLayerMoveDown();
    void onLayerToTop();
    void onLayerToBottom();
    void onLayerSelectionChanged();
    void onLayerVisibilityChanged(QListWidgetItem* pItem);

private:
    void createMenus();
    void createToolBar();
    void createCentralWidget();
    void createStatusBar();
    void rebuildProjectTree();
    void updateCanvasEditor();
    void syncCanvasToModel();
    void refreshLayerList();
    void updateWindowTitle();
    void openProjectPath(const QString& strJsonPath);
    void refreshRecentProjectsMenu();
    QString selectedPageKey() const;
    Page* currentPage();
    // 图层交换辅助：按方向交换相邻图层的 zOrder
    void moveLayer(int nOffset);
    void moveLayerTo(int nTargetIndex);

    ProjectManager* m_pProjectManager = nullptr;        // 项目管理
    CanvasScene* m_pScene = nullptr;                    // 画布场景
    CanvasView* m_pView = nullptr;                      // 画布视图
    QTreeWidget* m_pProjectTree = nullptr;              // 项目树
    QListWidget* m_pLayerList = nullptr;                // 图层面板
    QMenu* m_pRecentProjectsMenu = nullptr;             // 最近项目菜单
    QToolBar* m_pToolBar = nullptr;                     // 工具栏
    bool m_bSyncingCanvas = false;                      // 防止同步时信号回环
    // 树节点 → 数据路径（"攻略索引:页面索引" 或 "攻略索引" 或 空=项目）
    QHash<QTreeWidgetItem*, QString> m_mapNodeKeys;
};

} // namespace bwm

#endif // BWM_APP_MAINWINDOW_H
