/**
 * @file MainWindow.h
 * @author zhangweimu
 * @brief 主窗口：菜单栏、工具栏、项目树、画布、图层面板、状态栏（M2a 画布版）。
 */
#ifndef BWM_APP_MAINWINDOW_H
#define BWM_APP_MAINWINDOW_H

#include <QHash>
#include <QMainWindow>
#include <QVector>

#include "core/Component.h"

class QGraphicsView;
class QListWidget;
class QListWidgetItem;
class QMenu;
class QToolBar;
class QTreeWidget;
class QTreeWidgetItem;
class QUndoStack;

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

protected:
    // 关闭前检查未保存更改（保存/不保存/取消）
    void closeEvent(QCloseEvent* pEvent) override;

private slots:
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
    void onProjectTreeSelectionChanged();
    void onProjectOpened();
    void onAutoSavePerformed(bool bOk, const QString& strMessage);
    // 攻略 / 页面管理
    void onTreeContextMenu(const QPoint& rPos);
    void onAddWalkthrough();
    void onAddPage();
    void onRenameNode();
    void onDeleteNode();
    // 模板
    void onSaveAsTemplate();
    void onImportTemplate();
    void onExportTemplate();
    // 装饰贴纸
    void onAddStickerComponent(int nIndex);

    // 插入组件
    void onAddImageComponent();
    void onAddTextComponent();
    void onAddShapeComponent(int nIndex);
    void onAddTableComponent();
    // 编辑
    void onDeleteSelected();
    void onSelectAllComponents();
    // 画布与模型同步
    void onCanvasComponentsChanged();
    void onCanvasSelectionChanged();
    // 编辑事务（撤销快照）
    void onComponentEditStarted();
    void onComponentEditFinished();
    // 复制/粘贴/剪切
    void onCopy();
    void onPaste();
    void onCut();
    // 对齐与分布（nAlign 见 E_ALIGN_* 常量）
    void onAlignComponents(int nAlign);
    void onDistributeComponents(bool bHorizontal);
    // 右键菜单
    void onCanvasContextMenu(const QPointF& rScenePos);
    // 素材库
    void onImportAssets();
    void onAssetDoubleClicked(QListWidgetItem* pItem);
    void onAssetContextMenu(const QPoint& rPos);
    // 吸附开关
    void onToggleSnapToGrid(bool bEnable);
    void onToggleSnapToGuides(bool bEnable);
    // 图层操作
    void onLayerMoveUp();
    void onLayerMoveDown();
    void onLayerToTop();
    void onLayerToBottom();
    void onLayerSelectionChanged();
    void onLayerVisibilityChanged(QListWidgetItem* pItem);

private:
    // 对齐类型常量（onAlignComponents 的参数）
    enum E_ALIGN_TYPE {
        E_ALIGN_LEFT = 0,     // 左对齐
        E_ALIGN_H_CENTER,     // 水平居中
        E_ALIGN_RIGHT,        // 右对齐
        E_ALIGN_TOP,          // 顶对齐
        E_ALIGN_V_CENTER,     // 垂直居中
        E_ALIGN_BOTTOM,       // 底对齐
    };

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
    QString selectedNodeKey() const;
    void selectNodeByKey(const QString& rKey);
    Page* currentPage();
    // 当前页组件快照（撤销命令用）
    QVector<Component> currentComponents();
    // 提交一个快照撤销命令（前后一致时不提交）
    void pushSnapshot(const QString& strText, const QVector<Component>& rBefore,
                      const QVector<Component>& rAfter);
    // 图层交换辅助：按方向交换相邻图层的 zOrder
    void moveLayer(int nOffset);
    void moveLayerTo(int nTargetIndex);
    // 将指定图元置顶/置底
    void setItemToTop(ComponentItem* pItem, bool bTop);
    // 右键菜单辅助
    ComponentItem* componentItemAt(const QPointF& rScenePos) const;
    // 应用当前主题（画布背景色等）
    void applyTheme();
    // 素材库：刷新缩略图列表
    void refreshAssetList();

    ProjectManager* m_pProjectManager = nullptr;        // 项目管理
    CanvasScene* m_pScene = nullptr;                    // 画布场景
    CanvasView* m_pView = nullptr;                      // 画布视图
    QTreeWidget* m_pProjectTree = nullptr;              // 项目树
    QListWidget* m_pLayerList = nullptr;                // 图层面板
    QMenu* m_pRecentProjectsMenu = nullptr;             // 最近项目菜单
    QToolBar* m_pToolBar = nullptr;                     // 工具栏
    QListWidget* m_pAssetList = nullptr;                // 素材库列表
    QUndoStack* m_pUndoStack = nullptr;                 // 撤销栈
    bool m_bSyncingCanvas = false;                      // 防止同步时信号回环
    bool m_bInEditTransaction = false;                  // 是否处于编辑事务中
    QVector<Component> m_editBeforeSnapshot;            // 编辑事务前的组件快照
    QVector<Component> m_vecClipboard;                  // 应用内剪贴板（跨页面/项目）
    // 树节点 → 数据路径（"攻略索引:页面索引" 或 "攻略索引" 或 空=项目）
    QHash<QTreeWidgetItem*, QString> m_mapNodeKeys;
};

} // namespace bwm

#endif // BWM_APP_MAINWINDOW_H
