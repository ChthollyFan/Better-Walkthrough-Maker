#pragma once

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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onOpenRecentProject();
    void onSaveProject();
    void onToggleAutoSave(bool enabled);
    void onProjectTreeSelectionChanged();
    void onProjectOpened();
    void onAutoSavePerformed(bool ok, const QString &message);

private:
    void createMenus();
    void createCentralWidget();
    void createStatusBar();
    void rebuildProjectTree();
    void updateCanvasPlaceholder();
    void updateWindowTitle();
    void openProjectPath(const QString &jsonPath);
    void refreshRecentProjectsMenu();
    QString selectedPageKey() const;

    ProjectManager *m_projectManager = nullptr;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    QTreeWidget *m_projectTree = nullptr;
    QMenu *m_recentProjectsMenu = nullptr;
    // 树节点 → 数据路径（"攻略索引:页面索引" 或 "攻略索引" 或 空=项目）
    QHash<QTreeWidgetItem *, QString> m_nodeKeys;
};

} // namespace bwm
