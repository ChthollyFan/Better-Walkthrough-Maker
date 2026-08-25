/**
 * @file MainWindow.h
 * @author zhangweimu
 * @brief 主窗口：菜单栏、项目树、画布占位、状态栏（M1 骨架版）。
 */
#ifndef BWM_APP_MAINWINDOW_H
#define BWM_APP_MAINWINDOW_H

#include <QHash>
#include <QMainWindow>

class QGraphicsScene;
class QGraphicsView;
class QMenu;
class QTreeWidget;
class QTreeWidgetItem;

namespace bwm {

class ProjectManager;

// 主窗口：菜单栏、项目树、画布占位、状态栏（规划第 5.2 节，M1 为骨架版本）。
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

private:
    void createMenus();
    void createCentralWidget();
    void createStatusBar();
    void rebuildProjectTree();
    void updateCanvasPlaceholder();
    void updateWindowTitle();
    void openProjectPath(const QString& strJsonPath);
    void refreshRecentProjectsMenu();
    QString selectedPageKey() const;

    ProjectManager* m_pProjectManager = nullptr;        // 项目管理
    QGraphicsScene* m_pScene = nullptr;                 // 画布场景
    QGraphicsView* m_pView = nullptr;                   // 画布视图
    QTreeWidget* m_pProjectTree = nullptr;              // 项目树
    QMenu* m_pRecentProjectsMenu = nullptr;             // 最近项目菜单
    // 树节点 → 数据路径（"攻略索引:页面索引" 或 "攻略索引" 或 空=项目）
    QHash<QTreeWidgetItem*, QString> m_mapNodeKeys;
};

} // namespace bwm

#endif // BWM_APP_MAINWINDOW_H
