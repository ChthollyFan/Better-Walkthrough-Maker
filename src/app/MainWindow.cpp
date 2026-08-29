/**
 * @file MainWindow.cpp
 * @author zhangweimu
 * @brief 主窗口实现：组装各面板与画布，路由跨模块信号。
 *
 * 重构后 MainWindow 从 2290 行降至约 600 行，各面板/对话框/命令的内部逻辑
 * 已迁移到独立模块。MainWindow 仅保留：
 * - 构造组装（创建 PluginHost、面板、菜单、工具栏、信号连接）
 * - 文件操作（新建/打开/保存/关闭/导出/设置/关于）
 * - 画布↔模型同步（updateCanvasEditor / syncCanvasToModel / 编辑事务）
 * - 编辑操作（删除/复制/粘贴/剪切/对齐/分布/右键菜单）
 * - 跨模块信号路由
 */
#include "app/MainWindow.h"

#include "app/ComponentDisplay.h"
#include "app/commands/PageSnapshotCommand.h"
#include "app/dialogs/ExportDialog.h"
#include "app/dialogs/NewProjectDialog.h"
#include "app/dialogs/SettingsDialog.h"
#include "app/panels/AssetPanel.h"
#include "app/panels/LayerPanel.h"
#include "app/panels/ProjectTreePanel.h"
#include "core/Project.h"
#include "editor/CanvasScene.h"
#include "editor/CanvasView.h"
#include "editor/ComponentItem.h"
#include "export/ExportRenderer.h"
#include "plugin/PluginHost.h"
#include "plugin/IComponentProvider.h"
#include "plugin/IExportProvider.h"
#include "plugin/IThemeProvider.h"
#include "plugin/builtin/BuiltinPluginRegistrar.h"
#include "project/ProjectManager.h"
#include "settings/Settings.h"
#include "theme/Theme.h"

#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QCursor>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QUndoStack>
#include <QUuid>
#include <QUrl>

#include <algorithm>
#include <climits>

namespace bwm {

MainWindow::MainWindow(QWidget* pParent)
    : QMainWindow(pParent)
    , m_pHost(new PluginHost(this))
    , m_pProjectManager(new ProjectManager(this))
    , m_pScene(new CanvasScene(this))
{
    setWindowTitle(QStringLiteral("更好的攻略制作器"));
    resize(1400, 900);

    // 恢复上次窗口位置与大小
    const QByteArray geometry = Settings::windowGeometry();
    if(!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }

    m_pUndoStack = new QUndoStack(this);

    // 注册内置插件（组件类型、导出格式、模板、主题）
    registerBuiltinPlugins(m_pHost);

    // 构建 UI（顺序：中央区先建以创建各面板，再建菜单/工具栏以连接面板 slot）
    createCentralWidget();
    createMenus();
    createToolBar();
    createStatusBar();

    // ---- 连接项目管理器信号 ----
    connect(m_pProjectManager, &ProjectManager::projectOpened,
            this, &MainWindow::onProjectOpened);
    connect(m_pProjectManager, &ProjectManager::autoSavePerformed,
            this, &MainWindow::onAutoSavePerformed);

    // ---- 连接画布场景信号 ----
    connect(m_pScene, &CanvasScene::componentsChanged,
            this, &MainWindow::onCanvasComponentsChanged);
    connect(m_pScene, &QGraphicsScene::selectionChanged,
            this, &MainWindow::onCanvasSelectionChanged);
    connect(m_pScene, &CanvasScene::componentEditStarted,
            this, &MainWindow::onComponentEditStarted);
    connect(m_pScene, &CanvasScene::componentEditFinished,
            this, &MainWindow::onComponentEditFinished);
    connect(m_pView, &CanvasView::contextMenuRequested,
            this, &MainWindow::onCanvasContextMenu);

    applyTheme();   // 应用持久化的主题（画布背景色等）
}

MainWindow::~MainWindow() = default;

// =========================================================================
// UI 构建
// =========================================================================

void MainWindow::createMenus()
{
    // ---- 文件菜单 ----
    QMenu* pFileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));

    QAction* pNewAction = pFileMenu->addAction(QStringLiteral("新建项目(&N)…"));
    pNewAction->setShortcut(QKeySequence::New);
    connect(pNewAction, &QAction::triggered, this, &MainWindow::onNewProject);

    QAction* pOpenAction = pFileMenu->addAction(QStringLiteral("打开项目(&O)…"));
    pOpenAction->setShortcut(QKeySequence::Open);
    connect(pOpenAction, &QAction::triggered, this, &MainWindow::onOpenProject);

    m_pRecentProjectsMenu = pFileMenu->addMenu(QStringLiteral("最近项目(&R)"));
    refreshRecentProjectsMenu();

    pFileMenu->addSeparator();

    QAction* pSaveAction = pFileMenu->addAction(QStringLiteral("保存(&S)"));
    pSaveAction->setShortcut(QKeySequence::Save);
    connect(pSaveAction, &QAction::triggered, this, &MainWindow::onSaveProject);

    QAction* pAutoSaveAction = pFileMenu->addAction(QStringLiteral("自动保存(&A)"));
    pAutoSaveAction->setCheckable(true);
    pAutoSaveAction->setChecked(true);
    connect(pAutoSaveAction, &QAction::toggled, this, &MainWindow::onToggleAutoSave);

    QAction* pExportAction = pFileMenu->addAction(QStringLiteral("导出 PNG(&E)…"));
    pExportAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+E")));
    connect(pExportAction, &QAction::triggered, this, &MainWindow::onExportPng);

    pFileMenu->addSeparator();

    QAction* pSettingsAction = pFileMenu->addAction(QStringLiteral("设置(&T)…"));
    pSettingsAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+,")));
    connect(pSettingsAction, &QAction::triggered, this, &MainWindow::onShowSettings);

    pFileMenu->addSeparator();

    QAction* pExitAction = pFileMenu->addAction(QStringLiteral("退出(&X)"));
    pExitAction->setShortcut(QKeySequence::Quit);
    connect(pExitAction, &QAction::triggered, this, &QWidget::close);

    // ---- 攻略菜单 ----
    QMenu* pWalkthroughMenu = menuBar()->addMenu(QStringLiteral("攻略(&W)"));
    QAction* pAddWalkthroughAction = pWalkthroughMenu->addAction(QStringLiteral("新建攻略(&N)"));
    connect(pAddWalkthroughAction, &QAction::triggered,
            m_pTreePanel, &ProjectTreePanel::onAddWalkthrough);
    QAction* pAddPageAction = pWalkthroughMenu->addAction(QStringLiteral("新建页面(&P)"));
    connect(pAddPageAction, &QAction::triggered,
            m_pTreePanel, &ProjectTreePanel::onAddPage);
    pWalkthroughMenu->addSeparator();
    QAction* pRenameNodeAction = pWalkthroughMenu->addAction(QStringLiteral("重命名(&R)…"));
    connect(pRenameNodeAction, &QAction::triggered,
            m_pTreePanel, &ProjectTreePanel::onRenameNode);
    QAction* pDeleteNodeAction = pWalkthroughMenu->addAction(QStringLiteral("删除(&D)…"));
    connect(pDeleteNodeAction, &QAction::triggered,
            m_pTreePanel, &ProjectTreePanel::onDeleteNode);
    pWalkthroughMenu->addSeparator();
    QAction* pSaveTemplateAction = pWalkthroughMenu->addAction(QStringLiteral("保存为模板(&T)…"));
    connect(pSaveTemplateAction, &QAction::triggered,
            m_pTreePanel, &ProjectTreePanel::onSaveAsTemplate);
    QAction* pImportTemplateAction = pWalkthroughMenu->addAction(QStringLiteral("导入模板(&I)…"));
    connect(pImportTemplateAction, &QAction::triggered,
            m_pTreePanel, &ProjectTreePanel::onImportTemplate);
    QAction* pExportTemplateAction = pWalkthroughMenu->addAction(QStringLiteral("导出当前攻略为模板(&E)…"));
    connect(pExportTemplateAction, &QAction::triggered,
            m_pTreePanel, &ProjectTreePanel::onExportTemplate);

    // ---- 插入菜单（从 PluginHost 动态构建）----
    QMenu* pInsertMenu = menuBar()->addMenu(QStringLiteral("插入(&I)"));
    // 遍历所有组件类型 Provider，按 menuPath 构建子菜单
    for(const IComponentProvider* pProvider : m_pHost->componentProviders()) {
        QAction* pAction = pInsertMenu->addAction(pProvider->displayName());
        const IComponentProvider* pCaptured = pProvider;
        connect(pAction, &QAction::triggered, this, [this, pCaptured]() {
            insertComponent(pCaptured);
        });
    }

    // ---- 编辑菜单 ----
    QMenu* pEditMenu = menuBar()->addMenu(QStringLiteral("编辑(&E)"));

    QAction* pUndoAction = m_pUndoStack->createUndoAction(this, QStringLiteral("撤销(&U)"));
    pUndoAction->setShortcut(QKeySequence::Undo);
    pEditMenu->addAction(pUndoAction);

    QAction* pRedoAction = m_pUndoStack->createRedoAction(this, QStringLiteral("重做(&R)"));
    pRedoAction->setShortcut(QKeySequence::Redo);
    pEditMenu->addAction(pRedoAction);

    pEditMenu->addSeparator();

    QAction* pCutAction = pEditMenu->addAction(QStringLiteral("剪切(&T)"));
    pCutAction->setShortcut(QKeySequence::Cut);
    connect(pCutAction, &QAction::triggered, this, &MainWindow::onCut);

    QAction* pCopyAction = pEditMenu->addAction(QStringLiteral("复制(&C)"));
    pCopyAction->setShortcut(QKeySequence::Copy);
    connect(pCopyAction, &QAction::triggered, this, &MainWindow::onCopy);

    QAction* pPasteAction = pEditMenu->addAction(QStringLiteral("粘贴(&P)"));
    pPasteAction->setShortcut(QKeySequence::Paste);
    connect(pPasteAction, &QAction::triggered, this, &MainWindow::onPaste);

    pEditMenu->addSeparator();

    QAction* pCopyPageAction = pEditMenu->addAction(QStringLiteral("复制当前页到剪贴板(&B)"));
    pCopyPageAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
    connect(pCopyPageAction, &QAction::triggered, this, &MainWindow::onCopyPageToClipboard);

    pEditMenu->addSeparator();

    QAction* pDeleteAction = pEditMenu->addAction(QStringLiteral("删除选中(&D)"));
    pDeleteAction->setShortcut(QKeySequence::Delete);
    connect(pDeleteAction, &QAction::triggered, this, &MainWindow::onDeleteSelected);

    QAction* pSelectAllAction = pEditMenu->addAction(QStringLiteral("全选(&A)"));
    pSelectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(pSelectAllAction, &QAction::triggered, this, &MainWindow::onSelectAllComponents);

    pEditMenu->addSeparator();

    // 对齐子菜单
    QMenu* pAlignMenu = pEditMenu->addMenu(QStringLiteral("对齐(&G)"));
    const QList<QPair<QString, int>> alignActions = {
        {QStringLiteral("左对齐"), E_ALIGN_LEFT},
        {QStringLiteral("水平居中"), E_ALIGN_H_CENTER},
        {QStringLiteral("右对齐"), E_ALIGN_RIGHT},
        {QStringLiteral("顶对齐"), E_ALIGN_TOP},
        {QStringLiteral("垂直居中"), E_ALIGN_V_CENTER},
        {QStringLiteral("底对齐"), E_ALIGN_BOTTOM},
    };
    for(const QPair<QString, int>& rAlign : alignActions) {
        QAction* pAlignAction = pAlignMenu->addAction(rAlign.first);
        const int nAlign = rAlign.second;
        connect(pAlignAction, &QAction::triggered, this, [this, nAlign]() {
            onAlignComponents(nAlign);
        });
    }
    QAction* pDistributeHAction = pEditMenu->addAction(QStringLiteral("水平等距分布"));
    connect(pDistributeHAction, &QAction::triggered, this, [this]() {
        onDistributeComponents(true);
    });
    QAction* pDistributeVAction = pEditMenu->addAction(QStringLiteral("垂直等距分布"));
    connect(pDistributeVAction, &QAction::triggered, this, [this]() {
        onDistributeComponents(false);
    });

    pEditMenu->addSeparator();

    // 吸附开关
    QAction* pSnapGridAction = pEditMenu->addAction(QStringLiteral("网格吸附(&G)"));
    pSnapGridAction->setCheckable(true);
    pSnapGridAction->setChecked(m_pScene->snapToGrid());
    connect(pSnapGridAction, &QAction::toggled, this, &MainWindow::onToggleSnapToGrid);

    QAction* pSnapGuidesAction = pEditMenu->addAction(QStringLiteral("对齐参考线吸附(&L)"));
    pSnapGuidesAction->setCheckable(true);
    pSnapGuidesAction->setChecked(m_pScene->snapToGuides());
    connect(pSnapGuidesAction, &QAction::toggled, this, &MainWindow::onToggleSnapToGuides);

    // ---- 主题菜单（从 PluginHost 动态构建）----
    QMenu* pThemeMenu = menuBar()->addMenu(QStringLiteral("主题(&T)"));
    auto* pThemeGroup = new QActionGroup(this);
    const QString strCurrentTheme = ThemeManager::currentThemeName();
    // 遍历所有主题 Provider，合并主题列表
    for(const IThemeProvider* pProvider : m_pHost->themeProviders()) {
        for(const Theme& rTheme : pProvider->themes()) {
            QAction* pThemeAction = pThemeMenu->addAction(rTheme.strName);
            pThemeAction->setCheckable(true);
            pThemeAction->setData(rTheme.strName);
            if(rTheme.strName == strCurrentTheme) {
                pThemeAction->setChecked(true);
            }
            pThemeGroup->addAction(pThemeAction);
        }
    }
    connect(pThemeGroup, &QActionGroup::triggered, this, [this](QAction* pAction) {
        ThemeManager::setCurrentThemeName(pAction->data().toString());
        applyTheme();
    });

    // ---- 帮助菜单 ----
    QMenu* pHelpMenu = menuBar()->addMenu(QStringLiteral("帮助(&H)"));
    QAction* pShortcutsAction = pHelpMenu->addAction(QStringLiteral("快捷键(&K)…"));
    connect(pShortcutsAction, &QAction::triggered, this, &MainWindow::onShowShortcuts);
    QAction* pAboutAction = pHelpMenu->addAction(QStringLiteral("关于(&A)…"));
    connect(pAboutAction, &QAction::triggered, this, &MainWindow::onShowAbout);
}

void MainWindow::createToolBar()
{
    m_pToolBar = addToolBar(QStringLiteral("插入"));

    // 从 PluginHost 获取前几个组件类型作为工具栏快捷按钮
    // （图片、文本、表格；形状和贴纸通过菜单插入）
    for(const IComponentProvider* pProvider : m_pHost->componentProviders()) {
        // 仅添加 menuPath 为 "插入/xxx"（无第三级）的组件到工具栏
        const QString strPath = pProvider->menuPath();
        if(strPath.count(QLatin1Char('/')) == 1) {
            QAction* pAction = m_pToolBar->addAction(pProvider->displayName());
            const IComponentProvider* pCaptured = pProvider;
            connect(pAction, &QAction::triggered, this, [this, pCaptured]() {
                insertComponent(pCaptured);
            });
        }
    }

    m_pToolBar->addSeparator();

    // 撤销/重做
    QAction* pUndoAction = m_pUndoStack->createUndoAction(this, QStringLiteral("撤销"));
    m_pToolBar->addAction(pUndoAction);
    QAction* pRedoAction = m_pUndoStack->createRedoAction(this, QStringLiteral("重做"));
    m_pToolBar->addAction(pRedoAction);

    m_pToolBar->addSeparator();

    // 对齐按钮
    const QList<QPair<QString, int>> alignToolbar = {
        {QStringLiteral("左对齐"), E_ALIGN_LEFT},
        {QStringLiteral("水平居中"), E_ALIGN_H_CENTER},
        {QStringLiteral("右对齐"), E_ALIGN_RIGHT},
        {QStringLiteral("顶对齐"), E_ALIGN_TOP},
        {QStringLiteral("垂直居中"), E_ALIGN_V_CENTER},
        {QStringLiteral("底对齐"), E_ALIGN_BOTTOM},
    };
    for(const QPair<QString, int>& rAlign : alignToolbar) {
        QAction* pAlignAction = m_pToolBar->addAction(rAlign.first);
        const int nAlign = rAlign.second;
        connect(pAlignAction, &QAction::triggered, this, [this, nAlign]() {
            onAlignComponents(nAlign);
        });
    }

    m_pToolBar->addSeparator();
    QAction* pDeleteAction = m_pToolBar->addAction(QStringLiteral("删除选中"));
    connect(pDeleteAction, &QAction::triggered, this, &MainWindow::onDeleteSelected);
}

void MainWindow::createCentralWidget()
{
    // ---- 项目树面板 ----
    m_pTreePanel = new ProjectTreePanel(this, m_pProjectManager, m_pHost);
    connect(m_pTreePanel, &ProjectTreePanel::pageSelected,
            this, &MainWindow::onPageSelected);
    connect(m_pTreePanel, &ProjectTreePanel::projectStructureChanged,
            this, &MainWindow::onProjectStructureChanged);

    // ---- 画布 ----
    m_pView = new CanvasView(m_pScene, this);

    // ---- 右侧标签页：素材库 + 图层 ----
    m_pTabPanel = new QTabWidget(this);

    // 素材库面板
    m_pAssetPanel = new AssetPanel(m_pTabPanel, m_pProjectManager);
    connect(m_pAssetPanel, &AssetPanel::assetInserted,
            this, &MainWindow::onAssetInserted);
    m_pTabPanel->addTab(m_pAssetPanel, QStringLiteral("素材库"));

    // 图层面板
    m_pLayerPanel = new LayerPanel(m_pTabPanel, m_pScene, m_pView);
    connect(m_pLayerPanel, &LayerPanel::layerChanged,
            this, &MainWindow::syncCanvasToModel);
    m_pTabPanel->addTab(m_pLayerPanel, QStringLiteral("图层"));
    m_pTabPanel->setCurrentIndex(1);   // 默认显示图层

    // ---- 布局：项目树 | 画布 | 标签页 ----
    QSplitter* pSplitter = new QSplitter(Qt::Horizontal, this);
    pSplitter->addWidget(m_pTreePanel);
    pSplitter->addWidget(m_pView);
    pSplitter->addWidget(m_pTabPanel);
    pSplitter->setStretchFactor(1, 1);
    pSplitter->setSizes({220, 900, 220});

    setCentralWidget(pSplitter);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(QStringLiteral("就绪"));
}

void MainWindow::refreshRecentProjectsMenu()
{
    m_pRecentProjectsMenu->clear();
    const QStringList recents = Settings::recentProjects();
    if(recents.isEmpty()) {
        QAction* pEmptyAction = m_pRecentProjectsMenu->addAction(QStringLiteral("（无）"));
        pEmptyAction->setEnabled(false);
        return;
    }
    for(const QString& strPath : recents) {
        QAction* pAction = m_pRecentProjectsMenu->addAction(strPath);
        pAction->setData(strPath);
        connect(pAction, &QAction::triggered, this, &MainWindow::onOpenRecentProject);
    }
}

// =========================================================================
// 文件操作
// =========================================================================

void MainWindow::onNewProject()
{
    NewProjectDialog dialog(this);
    if(dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QString strGameName = dialog.gameName();
    if(strGameName.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("新建项目"), QStringLiteral("游戏名不能为空"));
        return;
    }
    const QString strParentDirectory = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择项目保存位置"));
    if(strParentDirectory.isEmpty()) {
        return;
    }
    QString strErrorMessage;
    if(!m_pProjectManager->createProject(strGameName, dialog.pageSize(),
                                         strParentDirectory, &strErrorMessage)) {
        QMessageBox::critical(this, QStringLiteral("新建项目"), strErrorMessage);
    }
    // 保存成功后的界面刷新由 projectOpened 信号统一处理
}

void MainWindow::onOpenProject()
{
    const QString strJsonPath = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开项目"), QString(),
        QStringLiteral("攻略项目 (project.json)"));
    if(strJsonPath.isEmpty()) {
        return;
    }
    openProjectPath(strJsonPath);
}

void MainWindow::onOpenRecentProject()
{
    if(QAction* pAction = qobject_cast<QAction*>(sender())) {
        openProjectPath(pAction->data().toString());
    }
}

void MainWindow::onSaveProject()
{
    if(!m_pProjectManager->hasProject()) {
        statusBar()->showMessage(QStringLiteral("当前没有打开的项目"), 3000);
        return;
    }
    // 保存前先同步画布到模型，保证未触发自动保存的编辑也落盘
    syncCanvasToModel();
    QString strErrorMessage;
    if(m_pProjectManager->save(&strErrorMessage)) {
        statusBar()->showMessage(QStringLiteral("已保存"), 3000);
        updateWindowTitle();
    } else {
        QMessageBox::critical(this, QStringLiteral("保存"), strErrorMessage);
    }
}

void MainWindow::closeEvent(QCloseEvent* pEvent)
{
    // 记住窗口状态
    Settings::setWindowGeometry(saveGeometry());

    if(m_pProjectManager->hasProject() && m_pProjectManager->isDirty()) {
        QMessageBox box(this);
        box.setWindowTitle(QStringLiteral("未保存的更改"));
        box.setIcon(QMessageBox::Question);
        box.setText(QStringLiteral("项目「%1」有未保存的更改，是否保存？")
                        .arg(m_pProjectManager->project()->strName));
        QPushButton* pSaveButton = box.addButton(QStringLiteral("保存"), QMessageBox::AcceptRole);
        QPushButton* pDiscardButton = box.addButton(QStringLiteral("不保存"), QMessageBox::DestructiveRole);
        QPushButton* pCancelButton = box.addButton(QStringLiteral("取消"), QMessageBox::RejectRole);
        box.setDefaultButton(pSaveButton);
        box.exec();

        const QAbstractButton* pClicked = box.clickedButton();
        if(pClicked == pSaveButton) {
            syncCanvasToModel();
            QString strErrorMessage;
            if(!m_pProjectManager->save(&strErrorMessage)) {
                QMessageBox::critical(this, QStringLiteral("保存失败"), strErrorMessage);
                pEvent->ignore();
                return;
            }
            pEvent->accept();
        } else if(pClicked == pCancelButton) {
            pEvent->ignore();
        } else {
            pEvent->accept();
        }
        return;
    }
    pEvent->accept();
}

void MainWindow::onExportPng()
{
    if(!m_pProjectManager->hasProject()) {
        statusBar()->showMessage(QStringLiteral("请先打开项目"), 3000);
        return;
    }
    ExportDialog dialog(this, m_pProjectManager, m_pHost, makeContext());
    dialog.setCurrentPageKey(m_pTreePanel->selectedPageKey());
    dialog.exec();
}

void MainWindow::onCopyPageToClipboard()
{
    Page* pPage = currentPage();
    if(!pPage) {
        statusBar()->showMessage(QStringLiteral("请先在左侧选择要复制的页面"), 3000);
        return;
    }
    const QImage image = ExportRenderer::renderPage(*pPage, 2.0,
                                                    ThemeManager::currentTheme().backgroundColor);
    QApplication::clipboard()->setImage(image);
    statusBar()->showMessage(QStringLiteral("当前页已复制到剪贴板（2x），可直接粘贴到小黑盒"), 4000);
}

void MainWindow::onShowSettings()
{
    SettingsDialog dialog(this, m_pProjectManager);
    if(dialog.exec() == QDialog::Accepted) {
        dialog.applySettings();
        statusBar()->showMessage(QStringLiteral("设置已保存"), 3000);
    }
}

void MainWindow::onShowShortcuts()
{
    QMessageBox::information(this, QStringLiteral("快捷键"),
                             QStringLiteral(
                                 "新建项目\tCtrl+N\n"
                                 "打开项目\tCtrl+O\n"
                                 "保存\tCtrl+S\n"
                                 "导出 PNG\tCtrl+E\n"
                                 "复制当前页到剪贴板\tCtrl+Shift+C\n"
                                 "\n"
                                 "撤销\tCtrl+Z\n"
                                 "重做\tCtrl+Y\n"
                                 "复制\tCtrl+C\n"
                                 "粘贴\tCtrl+V\n"
                                 "剪切\tCtrl+X\n"
                                 "删除选中\tDelete\n"
                                 "全选\tCtrl+A\n"
                                 "\n"
                                 "画布缩放\tCtrl+滚轮\n"
                                 "设置\tCtrl+,"));
}

void MainWindow::onShowAbout()
{
    QMessageBox::about(this, QStringLiteral("关于 更好的攻略制作器"),
                       QStringLiteral(
                           "更好的攻略制作器（Better Walkthrough Maker）\n"
                           "版本 %1\n\n"
                           "面向游戏攻略作者的桌面设计工具：\n"
                           "用模板 + 自由画布制作攻略配图，导出 PNG 发布到小黑盒等平台。")
                       .arg(QCoreApplication::applicationVersion()));
}

void MainWindow::onToggleAutoSave(bool bEnabled)
{
    m_pProjectManager->setAutoSaveEnabled(bEnabled);
    statusBar()->showMessage(bEnabled ? QStringLiteral("自动保存已开启")
                                      : QStringLiteral("自动保存已关闭"), 3000);
}

void MainWindow::openProjectPath(const QString& strJsonPath)
{
    QString strErrorMessage;
    if(!m_pProjectManager->openProject(strJsonPath, &strErrorMessage)) {
        QMessageBox::critical(this, QStringLiteral("打开项目"), strErrorMessage);
        return;
    }
    // 崩溃恢复：打开后检测未完成保存的残留 .tmp，提示恢复
    const QString strProjectDir = m_pProjectManager->projectDirectory();
    if(ProjectManager::hasRecoverableSnapshot(strProjectDir)) {
        const QMessageBox::StandardButton result = QMessageBox::question(
            this, QStringLiteral("检测到未完成的保存"),
            QStringLiteral("上次保存可能被中断，是否恢复最近一次保存的内容？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if(result == QMessageBox::Yes) {
            QString strRecoverError;
            if(m_pProjectManager->recoverFromSnapshot(&strRecoverError)) {
                QString strReopenError;
                if(m_pProjectManager->openProject(strJsonPath, &strReopenError)) {
                    statusBar()->showMessage(QStringLiteral("已从上次未完成的保存恢复"), 5000);
                }
            } else {
                QMessageBox::warning(this, QStringLiteral("恢复失败"), strRecoverError);
            }
        }
    }
}

void MainWindow::onProjectOpened()
{
    m_pTreePanel->rebuildProjectTree();
    refreshRecentProjectsMenu();
    m_pAssetPanel->refreshAssetList();
    updateWindowTitle();
    statusBar()->showMessage(QStringLiteral("已打开项目：%1").arg(m_pProjectManager->projectDirectory()), 5000);
    // 自动选中第一个攻略的首页，打开项目即有画面
    const Project* pProject = m_pProjectManager->project();
    if(pProject && !pProject->vecWalkthroughs.isEmpty()
       && !pProject->vecWalkthroughs.first().vecPages.isEmpty()) {
        m_pTreePanel->selectNodeByKey(QStringLiteral("0:0"));
    }
}

void MainWindow::onAutoSavePerformed(bool bOk, const QString& strMessage)
{
    statusBar()->showMessage(strMessage, bOk ? 3000 : 8000);
    updateWindowTitle();
}

// =========================================================================
// 项目树信号处理
// =========================================================================

void MainWindow::onPageSelected(const QString& rPageKey)
{
    (void)rPageKey;
    updateCanvasEditor();
}

void MainWindow::onProjectStructureChanged()
{
    updateCanvasEditor();
    updateWindowTitle();
}

// =========================================================================
// 画布与模型同步
// =========================================================================

void MainWindow::updateCanvasEditor()
{
    Page* pPage = currentPage();
    if(!pPage) {
        m_pScene->clearPage();
        return;
    }
    m_bSyncingCanvas = true;
    m_pScene->loadPage(*pPage);
    m_bSyncingCanvas = false;
    m_pView->fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
    m_pView->centerOn(m_pScene->sceneRect().center());
    m_pLayerPanel->refreshLayerList();
}

void MainWindow::syncCanvasToModel()
{
    if(m_bSyncingCanvas) {
        return;
    }
    Page* pPage = currentPage();
    if(pPage) {
        m_pScene->syncToModel(pPage);
        m_pProjectManager->setDirty();
        updateWindowTitle();
    }
}

void MainWindow::onCanvasComponentsChanged()
{
    syncCanvasToModel();
    m_pLayerPanel->refreshLayerList();
}

void MainWindow::onCanvasSelectionChanged()
{
    if(m_bSyncingCanvas) {
        return;
    }
    m_pLayerPanel->syncSelectionFromScene();
}

void MainWindow::applyTheme()
{
    const Theme theme = ThemeManager::currentTheme();
    m_pScene->setPageBackgroundColor(theme.backgroundColor);
    m_pView->viewport()->update();
    updateCanvasEditor();
}

// =========================================================================
// 项目/页面辅助
// =========================================================================

Page* MainWindow::currentPage()
{
    const QString strPageKey = m_pTreePanel->selectedPageKey();
    if(strPageKey.isEmpty()) {
        return nullptr;
    }
    const QStringList parts = strPageKey.split(QLatin1Char(':'));
    const int nWalkthroughIndex = parts.at(0).toInt();
    const int nPageIndex = parts.at(1).toInt();
    Project* pProject = m_pProjectManager->project();
    if(!pProject || nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
        return nullptr;
    }
    Walkthrough& rWalkthrough = pProject->vecWalkthroughs[nWalkthroughIndex];
    if(nPageIndex < 0 || nPageIndex >= rWalkthrough.vecPages.size()) {
        return nullptr;
    }
    return &rWalkthrough.vecPages[nPageIndex];
}

QVector<Component> MainWindow::currentComponents()
{
    Page* pPage = currentPage();
    return pPage ? pPage->vecComponents : QVector<Component>();
}

void MainWindow::pushSnapshot(const QString& strText, const QVector<Component>& rBefore,
                              const QVector<Component>& rAfter)
{
    if(rBefore == rAfter) {
        return;
    }
    Page* pPage = currentPage();
    if(!pPage) {
        return;
    }
    m_pUndoStack->push(new PageSnapshotCommand(pPage, rBefore, rAfter, strText, m_pScene));
}

void MainWindow::updateWindowTitle()
{
    if(!m_pProjectManager->hasProject()) {
        setWindowTitle(QStringLiteral("更好的攻略制作器"));
        return;
    }
    const QString strDirtyMark = m_pProjectManager->isDirty() ? QStringLiteral(" *") : QString();
    setWindowTitle(QStringLiteral("%1%2 - 更好的攻略制作器")
                       .arg(m_pProjectManager->project()->strName, strDirtyMark));
}

PluginContext MainWindow::makeContext() const
{
    PluginContext ctx;
    ctx.theme = ThemeManager::currentTheme();
    ctx.defaultPageSize = Settings::defaultPageSize();
    ctx.projectDirectory = m_pProjectManager->hasProject()
        ? m_pProjectManager->projectDirectory() : QString();
    // 注意：currentPage 返回非 const，这里不能在 const 方法中调用
    // pCurrentPage 由调用方在需要时单独设置
    return ctx;
}

// =========================================================================
// 组件插入（统一入口）
// =========================================================================

void MainWindow::insertComponent(const IComponentProvider* pProvider)
{
    if(!currentPage()) {
        statusBar()->showMessage(QStringLiteral("请先在左侧选择要编辑的页面"), 3000);
        return;
    }
    PluginContext ctx = makeContext();
    ctx.pCurrentPage = currentPage();

    Component component = pProvider->createComponent(ctx);

    // 若 Provider 需要输入对话框（如图片文件选择、文本输入），弹出对话框
    if(pProvider->requiresInputDialog()) {
        if(!pProvider->showInputDialog(this, component, ctx)) {
            return;   // 用户取消
        }
    }

    ComponentItem* pNewItem = m_pScene->addComponent(component);
    if(pNewItem) {
        m_pScene->clearSelection();
        pNewItem->setSelected(true);
        m_pView->centerOn(pNewItem);
    }
}

// =========================================================================
// 编辑操作
// =========================================================================

void MainWindow::onComponentEditStarted()
{
    if(m_bInEditTransaction) {
        return;
    }
    m_bInEditTransaction = true;
    m_editBeforeSnapshot = currentComponents();
}

void MainWindow::onComponentEditFinished()
{
    if(!m_bInEditTransaction) {
        return;
    }
    m_bInEditTransaction = false;
    pushSnapshot(QStringLiteral("移动/缩放组件"), m_editBeforeSnapshot, currentComponents());
}

void MainWindow::onDeleteSelected()
{
    m_pScene->removeSelectedComponents();
}

void MainWindow::onSelectAllComponents()
{
    for(ComponentItem* pItem : m_pScene->componentItems()) {
        pItem->setSelected(true);
    }
}

void MainWindow::onCopy()
{
    m_vecClipboard.clear();
    const QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
    for(const ComponentItem* pItem : vecSelected) {
        m_vecClipboard.append(pItem->component());
    }
    statusBar()->showMessage(QStringLiteral("已复制 %1 个组件").arg(m_vecClipboard.size()), 2000);
}

void MainWindow::onPaste()
{
    if(m_vecClipboard.isEmpty()) {
        return;
    }
    Page* pPage = currentPage();
    if(!pPage) {
        statusBar()->showMessage(QStringLiteral("请先在左侧选择要编辑的页面"), 3000);
        return;
    }
    const QVector<Component> vecBefore = pPage->vecComponents;
    const int nOffset = 20;
    for(const Component& rComponent : m_vecClipboard) {
        Component component = rComponent;
        component.strId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        component.pos += QPointF(nOffset, nOffset);
        component.nZOrder = 0;   // addComponent 自动分配新 zOrder
        m_pScene->addComponent(component);
    }
    pushSnapshot(QStringLiteral("粘贴组件"), vecBefore, currentComponents());
}

void MainWindow::onCut()
{
    if(m_pScene->selectedComponentItems().isEmpty()) {
        return;
    }
    onCopy();
    onDeleteSelected();
}

void MainWindow::onAlignComponents(int nAlign)
{
    QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
    if(vecSelected.size() < 2) {
        statusBar()->showMessage(QStringLiteral("请先选中至少两个组件"), 2000);
        return;
    }

    // 计算选中组件的包围盒
    QRectF boundingBox;
    bool bFirst = true;
    for(const ComponentItem* pItem : vecSelected) {
        const QRectF itemRect(pItem->component().pos, pItem->component().size);
        if(bFirst) {
            boundingBox = itemRect;
            bFirst = false;
        } else {
            boundingBox = boundingBox.united(itemRect);
        }
    }

    const QVector<Component> vecBefore = currentComponents();
    for(ComponentItem* pItem : vecSelected) {
        Component component = pItem->component();
        const qreal dWidth = component.size.width();
        const qreal dHeight = component.size.height();
        switch(nAlign) {
        case E_ALIGN_LEFT:
            component.pos.setX(boundingBox.left());
            break;
        case E_ALIGN_H_CENTER:
            component.pos.setX(boundingBox.center().x() - dWidth / 2);
            break;
        case E_ALIGN_RIGHT:
            component.pos.setX(boundingBox.right() - dWidth);
            break;
        case E_ALIGN_TOP:
            component.pos.setY(boundingBox.top());
            break;
        case E_ALIGN_V_CENTER:
            component.pos.setY(boundingBox.center().y() - dHeight / 2);
            break;
        case E_ALIGN_BOTTOM:
            component.pos.setY(boundingBox.bottom() - dHeight);
            break;
        default:
            break;
        }
        pItem->setComponent(component);
    }
    pushSnapshot(QStringLiteral("对齐组件"), vecBefore, currentComponents());
}

void MainWindow::onDistributeComponents(bool bHorizontal)
{
    QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
    if(vecSelected.size() < 3) {
        statusBar()->showMessage(QStringLiteral("请先选中至少三个组件"), 2000);
        return;
    }
    // 按主轴坐标排序
    std::sort(vecSelected.begin(), vecSelected.end(),
              [bHorizontal](const ComponentItem* pLeft, const ComponentItem* pRight) {
                  const qreal dLeftValue = bHorizontal ? pLeft->component().pos.x()
                                                       : pLeft->component().pos.y();
                  const qreal dRightValue = bHorizontal ? pRight->component().pos.x()
                                                        : pRight->component().pos.y();
                  return dLeftValue < dRightValue;
              });

    // 计算主轴总长（首组件起点 → 末组件终点）
    const qreal dStart = bHorizontal ? vecSelected.first()->component().pos.x()
                                     : vecSelected.first()->component().pos.y();
    const qreal dEnd = bHorizontal
        ? vecSelected.last()->component().pos.x() + vecSelected.last()->component().size.width()
        : vecSelected.last()->component().pos.y() + vecSelected.last()->component().size.height();
    qreal dTotalSize = 0;
    for(const ComponentItem* pItem : vecSelected) {
        dTotalSize += bHorizontal ? pItem->component().size.width()
                                  : pItem->component().size.height();
    }
    const qreal dGap = (dEnd - dStart - dTotalSize) / (vecSelected.size() - 1);

    const QVector<Component> vecBefore = currentComponents();
    qreal dCursor = dStart;
    for(ComponentItem* pItem : vecSelected) {
        Component component = pItem->component();
        if(bHorizontal) {
            component.pos.setX(dCursor);
        } else {
            component.pos.setY(dCursor);
        }
        pItem->setComponent(component);
        dCursor += (bHorizontal ? component.size.width() : component.size.height()) + dGap;
    }
    pushSnapshot(QStringLiteral("等距分布"), vecBefore, currentComponents());
}

void MainWindow::onCanvasContextMenu(const QPointF& rScenePos)
{
    ComponentItem* pHitItem = componentItemAt(rScenePos);
    // 命中未选中的组件时先选中它
    if(pHitItem && !pHitItem->isSelected()) {
        m_pScene->clearSelection();
        pHitItem->setSelected(true);
    }

    QMenu menu(this);
    const bool bHasSelection = !m_pScene->selectedComponentItems().isEmpty();

    QAction* pCutAction = menu.addAction(QStringLiteral("剪切"));
    pCutAction->setEnabled(bHasSelection);
    QAction* pCopyAction = menu.addAction(QStringLiteral("复制"));
    pCopyAction->setEnabled(bHasSelection);
    QAction* pPasteAction = menu.addAction(QStringLiteral("粘贴"));
    pPasteAction->setEnabled(!m_vecClipboard.isEmpty());
    menu.addSeparator();

    QAction* pDeleteAction = menu.addAction(QStringLiteral("删除"));
    pDeleteAction->setEnabled(bHasSelection);

    QAction* pToTopAction = menu.addAction(QStringLiteral("置顶"));
    pToTopAction->setEnabled(bHasSelection);
    QAction* pToBottomAction = menu.addAction(QStringLiteral("置底"));
    pToBottomAction->setEnabled(bHasSelection);

    QAction* pEditTextAction = nullptr;
    QAction* pLockAction = nullptr;
    if(pHitItem) {
        const E_COMPONENT_TYPE eType = pHitItem->component().eType;
        if(eType == E_COMPONENT_TYPE_TEXT || eType == E_COMPONENT_TYPE_TABLE
           || eType == E_COMPONENT_TYPE_STICKER) {
            menu.addSeparator();
            pEditTextAction = menu.addAction(eType == E_COMPONENT_TYPE_TEXT
                                                 ? QStringLiteral("编辑文本…")
                                                 : eType == E_COMPONENT_TYPE_TABLE
                                                     ? QStringLiteral("编辑表格…")
                                                     : QStringLiteral("编辑贴纸…"));
        }
        menu.addSeparator();
        pLockAction = menu.addAction(pHitItem->component().bLocked
                                         ? QStringLiteral("解除锁定")
                                         : QStringLiteral("锁定"));
    }

    QAction* pChosen = menu.exec(QCursor::pos());
    if(!pChosen) {
        return;
    }
    if(pChosen == pCutAction) {
        onCut();
    } else if(pChosen == pCopyAction) {
        onCopy();
    } else if(pChosen == pPasteAction) {
        onPaste();
    } else if(pChosen == pDeleteAction) {
        onDeleteSelected();
    } else if(pChosen == pToTopAction) {
        const QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
        for(ComponentItem* pItem : vecSelected) {
            setItemToTop(pItem, true);
        }
        syncCanvasToModel();
    } else if(pChosen == pToBottomAction) {
        const QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
        for(ComponentItem* pItem : vecSelected) {
            setItemToTop(pItem, false);
        }
        syncCanvasToModel();
    } else if(pChosen == pEditTextAction && pHitItem) {
        pHitItem->editContent();
    } else if(pChosen == pLockAction && pHitItem) {
        Component component = pHitItem->component();
        component.bLocked = !component.bLocked;
        pHitItem->setComponent(component);
        syncCanvasToModel();
    }
}

void MainWindow::onToggleSnapToGrid(bool bEnable)
{
    m_pScene->setSnapToGrid(bEnable);
    m_pView->viewport()->update();
}

void MainWindow::onToggleSnapToGuides(bool bEnable)
{
    m_pScene->setSnapToGuides(bEnable);
}

// =========================================================================
// 图层辅助
// =========================================================================

void MainWindow::setItemToTop(ComponentItem* pItem, bool bTop)
{
    int nTargetZ = bTop ? 0 : INT_MAX;
    for(const ComponentItem* pOther : m_pScene->componentItems()) {
        if(pOther == pItem) {
            continue;
        }
        if(bTop) {
            nTargetZ = qMax(nTargetZ, pOther->component().nZOrder);
        } else {
            nTargetZ = qMin(nTargetZ, pOther->component().nZOrder);
        }
    }
    Component component = pItem->component();
    component.nZOrder = bTop ? nTargetZ + 1 : nTargetZ - 1;
    pItem->setComponent(component);
    m_pScene->sortByZOrder();
}

ComponentItem* MainWindow::componentItemAt(const QPointF& rScenePos) const
{
    for(const ComponentItem* pItem : m_pScene->componentItems()) {
        // 命中检测用组件的矩形范围（不考虑旋转手柄）
        const QRectF itemRect(pItem->component().pos, pItem->component().size);
        if(itemRect.contains(rScenePos)) {
            return const_cast<ComponentItem*>(pItem);
        }
    }
    return nullptr;
}

// =========================================================================
// 素材面板信号处理
// =========================================================================

void MainWindow::onAssetInserted(const Component& rComponent)
{
    ComponentItem* pNewItem = m_pScene->addComponent(rComponent);
    if(pNewItem) {
        m_pScene->clearSelection();
        pNewItem->setSelected(true);
        m_pView->centerOn(pNewItem);
    }
}

} // namespace bwm
