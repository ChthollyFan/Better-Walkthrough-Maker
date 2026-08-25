/**
 * @file MainWindow.cpp
 * @author zhangweimu
 * @brief 主窗口实现（M2a：画布编辑 + 图层面板）。
 */
#include "app/MainWindow.h"

#include "core/Project.h"
#include "editor/CanvasScene.h"
#include "editor/CanvasView.h"
#include "editor/ComponentItem.h"
#include "project/ProjectManager.h"
#include "settings/Settings.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCursor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeWidget>
#include <QUndoStack>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <climits>

namespace bwm {

namespace {

// 图层列表项中保存 ComponentItem* 的 UserRole
constexpr int nComponentItemRole = Qt::UserRole + 1;

// 组件类型的显示名称
QString componentDisplayName(const Component& rComponent)
{
    switch (rComponent.eType) {
    case E_COMPONENT_TYPE_IMAGE:
        return QStringLiteral("图片");
    case E_COMPONENT_TYPE_TEXT:
        return rComponent.textData.strContent.left(12);
    case E_COMPONENT_TYPE_SHAPE:
    default:
        return QStringLiteral("形状");
    }
}

// 基于页面组件快照的撤销命令：undo/redo 时整体恢复组件列表并重建场景。
class PageSnapshotCommand : public QUndoCommand {
public:
    PageSnapshotCommand(Page* pPage, const QVector<Component>& rBefore,
                        const QVector<Component>& rAfter, const QString& strText,
                        CanvasScene* pScene)
        : QUndoCommand(strText)
        , m_pPage(pPage)
        , m_vecBefore(rBefore)
        , m_vecAfter(rAfter)
        , m_pScene(pScene)
    {
    }

    void undo() override
    {
        apply(m_vecBefore);
    }

    void redo() override
    {
        apply(m_vecAfter);
    }

private:
    void apply(const QVector<Component>& rComponents)
    {
        // 模型已是目标状态时无需重建（如 QUndoStack::push 时 redo 与现态一致）
        if (m_pPage->vecComponents == rComponents) {
            return;
        }
        m_pPage->vecComponents = rComponents;
        m_pScene->loadPage(*m_pPage);
        emit m_pScene->componentsChanged();   // 触发主窗口同步与图层刷新
    }

    Page* m_pPage;
    QVector<Component> m_vecBefore;
    QVector<Component> m_vecAfter;
    CanvasScene* m_pScene;
};

} // namespace

MainWindow::MainWindow(QWidget* pParent)
    : QMainWindow(pParent)
    , m_pProjectManager(new ProjectManager(this))
    , m_pScene(new CanvasScene(this))
{
    setWindowTitle(QStringLiteral("更好的攻略制作器"));
    resize(1400, 900);

    m_pUndoStack = new QUndoStack(this);

    createMenus();
    createToolBar();
    createCentralWidget();
    createStatusBar();

    connect(m_pProjectManager, &ProjectManager::projectOpened,
            this, &MainWindow::onProjectOpened);
    connect(m_pProjectManager, &ProjectManager::autoSavePerformed,
            this, &MainWindow::onAutoSavePerformed);
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
}

MainWindow::~MainWindow() = default;

void MainWindow::createMenus()
{
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

    pFileMenu->addSeparator();

    QAction* pExitAction = pFileMenu->addAction(QStringLiteral("退出(&X)"));
    pExitAction->setShortcut(QKeySequence::Quit);
    connect(pExitAction, &QAction::triggered, this, &QWidget::close);

    // 插入菜单
    QMenu* pInsertMenu = menuBar()->addMenu(QStringLiteral("插入(&I)"));

    QAction* pAddImageAction = pInsertMenu->addAction(QStringLiteral("图片(&P)…"));
    connect(pAddImageAction, &QAction::triggered, this, &MainWindow::onAddImageComponent);

    QAction* pAddTextAction = pInsertMenu->addAction(QStringLiteral("文本(&T)…"));
    connect(pAddTextAction, &QAction::triggered, this, &MainWindow::onAddTextComponent);

    QMenu* pShapeMenu = pInsertMenu->addMenu(QStringLiteral("形状(&S)"));
    QAction* pAddRectAction = pShapeMenu->addAction(QStringLiteral("矩形"));
    pAddRectAction->setData(static_cast<int>(E_SHAPE_TYPE_RECTANGLE));
    connect(pAddRectAction, &QAction::triggered, this, [this]() {
        onAddShapeComponent(static_cast<int>(E_SHAPE_TYPE_RECTANGLE));
    });
    QAction* pAddRoundRectAction = pShapeMenu->addAction(QStringLiteral("圆角矩形"));
    pAddRoundRectAction->setData(static_cast<int>(E_SHAPE_TYPE_ROUND_RECT));
    connect(pAddRoundRectAction, &QAction::triggered, this, [this]() {
        onAddShapeComponent(static_cast<int>(E_SHAPE_TYPE_ROUND_RECT));
    });
    QAction* pAddEllipseAction = pShapeMenu->addAction(QStringLiteral("椭圆"));
    pAddEllipseAction->setData(static_cast<int>(E_SHAPE_TYPE_ELLIPSE));
    connect(pAddEllipseAction, &QAction::triggered, this, [this]() {
        onAddShapeComponent(static_cast<int>(E_SHAPE_TYPE_ELLIPSE));
    });
    QAction* pAddLineAction = pShapeMenu->addAction(QStringLiteral("线条"));
    pAddLineAction->setData(static_cast<int>(E_SHAPE_TYPE_LINE));
    connect(pAddLineAction, &QAction::triggered, this, [this]() {
        onAddShapeComponent(static_cast<int>(E_SHAPE_TYPE_LINE));
    });

    // 编辑菜单
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
    for (const QPair<QString, int>& rAlign : alignActions) {
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
}

void MainWindow::createToolBar()
{
    m_pToolBar = addToolBar(QStringLiteral("插入"));

    QAction* pAddImageAction = m_pToolBar->addAction(QStringLiteral("插入图片"));
    connect(pAddImageAction, &QAction::triggered, this, &MainWindow::onAddImageComponent);

    QAction* pAddTextAction = m_pToolBar->addAction(QStringLiteral("插入文本"));
    connect(pAddTextAction, &QAction::triggered, this, &MainWindow::onAddTextComponent);

    auto* pShapeCombo = new QComboBox(m_pToolBar);
    pShapeCombo->addItem(QStringLiteral("矩形"), static_cast<int>(E_SHAPE_TYPE_RECTANGLE));
    pShapeCombo->addItem(QStringLiteral("圆角矩形"), static_cast<int>(E_SHAPE_TYPE_ROUND_RECT));
    pShapeCombo->addItem(QStringLiteral("椭圆"), static_cast<int>(E_SHAPE_TYPE_ELLIPSE));
    pShapeCombo->addItem(QStringLiteral("线条"), static_cast<int>(E_SHAPE_TYPE_LINE));
    m_pToolBar->addWidget(pShapeCombo);
    QAction* pAddShapeAction = m_pToolBar->addAction(QStringLiteral("插入形状"));
    connect(pAddShapeAction, &QAction::triggered, this, [this, pShapeCombo]() {
        onAddShapeComponent(pShapeCombo->currentData().toInt());
    });

    m_pToolBar->addSeparator();

    QAction* pUndoAction = m_pUndoStack->createUndoAction(this, QStringLiteral("撤销"));
    m_pToolBar->addAction(pUndoAction);
    QAction* pRedoAction = m_pUndoStack->createRedoAction(this, QStringLiteral("重做"));
    m_pToolBar->addAction(pRedoAction);

    m_pToolBar->addSeparator();

    // 对齐按钮（选中多个组件时有效）
    const QList<QPair<QString, int>> alignToolbar = {
        {QStringLiteral("左对齐"), E_ALIGN_LEFT},
        {QStringLiteral("水平居中"), E_ALIGN_H_CENTER},
        {QStringLiteral("右对齐"), E_ALIGN_RIGHT},
        {QStringLiteral("顶对齐"), E_ALIGN_TOP},
        {QStringLiteral("垂直居中"), E_ALIGN_V_CENTER},
        {QStringLiteral("底对齐"), E_ALIGN_BOTTOM},
    };
    for (const QPair<QString, int>& rAlign : alignToolbar) {
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
    m_pProjectTree = new QTreeWidget(this);
    m_pProjectTree->setHeaderLabel(QStringLiteral("项目结构"));
    m_pProjectTree->setMinimumWidth(200);
    connect(m_pProjectTree, &QTreeWidget::itemSelectionChanged,
            this, &MainWindow::onProjectTreeSelectionChanged);

    m_pView = new CanvasView(m_pScene, this);

    // 图层面板：列表 + 操作按钮
    auto* pLayerPanel = new QWidget(this);
    auto* pLayerLayout = new QVBoxLayout(pLayerPanel);
    pLayerLayout->setContentsMargins(0, 0, 0, 0);
    m_pLayerList = new QListWidget(pLayerPanel);
    m_pLayerList->setMinimumWidth(180);
    connect(m_pLayerList, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onLayerSelectionChanged);
    connect(m_pLayerList, &QListWidget::itemChanged,
            this, &MainWindow::onLayerVisibilityChanged);
    pLayerLayout->addWidget(m_pLayerList);

    auto* pLayerButtons = new QHBoxLayout;
    QPushButton* pMoveUpButton = new QPushButton(QStringLiteral("上移"), pLayerPanel);
    QPushButton* pMoveDownButton = new QPushButton(QStringLiteral("下移"), pLayerPanel);
    QPushButton* pToTopButton = new QPushButton(QStringLiteral("置顶"), pLayerPanel);
    QPushButton* pToBottomButton = new QPushButton(QStringLiteral("置底"), pLayerPanel);
    connect(pMoveUpButton, &QPushButton::clicked, this, &MainWindow::onLayerMoveUp);
    connect(pMoveDownButton, &QPushButton::clicked, this, &MainWindow::onLayerMoveDown);
    connect(pToTopButton, &QPushButton::clicked, this, &MainWindow::onLayerToTop);
    connect(pToBottomButton, &QPushButton::clicked, this, &MainWindow::onLayerToBottom);
    pLayerButtons->addWidget(pMoveUpButton);
    pLayerButtons->addWidget(pMoveDownButton);
    pLayerButtons->addWidget(pToTopButton);
    pLayerButtons->addWidget(pToBottomButton);
    pLayerLayout->addLayout(pLayerButtons);

    QSplitter* pSplitter = new QSplitter(Qt::Horizontal, this);
    pSplitter->addWidget(m_pProjectTree);
    pSplitter->addWidget(m_pView);
    pSplitter->addWidget(pLayerPanel);
    pSplitter->setStretchFactor(1, 1);
    pSplitter->setSizes({220, 900, 220});

    setCentralWidget(pSplitter);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(QStringLiteral("就绪"));
}

void MainWindow::onNewProject()
{
    // 新建项目向导（M1 基础版：游戏名 + 画布尺寸）
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("新建项目"));

    auto* pNameEdit = new QLineEdit(&dialog);
    pNameEdit->setPlaceholderText(QStringLiteral("游戏名，如：艾尔登法环"));

    auto* pSizeCombo = new QComboBox(&dialog);
    const QSize defaultSize = Settings::defaultPageSize();
    const QList<QPair<QString, QSize>> presets = {
        {QStringLiteral("竖图 1080×1440（默认）"), QSize(1080, 1440)},
        {QStringLiteral("横图 1920×1080"), QSize(1920, 1080)},
        {QStringLiteral("方形 1080×1080"), QSize(1080, 1080)},
        {QStringLiteral("长图 1080×2400"), QSize(1080, 2400)},
    };
    int nSelectedIndex = 0;
    for (int nIndex = 0; nIndex < presets.size(); ++nIndex) {
        pSizeCombo->addItem(presets.at(nIndex).first, presets.at(nIndex).second);
        if (presets.at(nIndex).second == defaultSize) {
            nSelectedIndex = nIndex;
        }
    }
    pSizeCombo->setCurrentIndex(nSelectedIndex);

    auto* pForm = new QFormLayout(&dialog);
    pForm->addRow(QStringLiteral("游戏名："), pNameEdit);
    pForm->addRow(QStringLiteral("默认画布尺寸："), pSizeCombo);

    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    pButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("创建"));
    pButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(pButtons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(pButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    pForm->addRow(pButtons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString strGameName = pNameEdit->text().trimmed();
    if (strGameName.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("新建项目"), QStringLiteral("游戏名不能为空"));
        return;
    }

    const QString strParentDirectory = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择项目保存位置"));
    if (strParentDirectory.isEmpty()) {
        return;
    }

    QString strErrorMessage;
    if (!m_pProjectManager->createProject(strGameName, pSizeCombo->currentData().toSize(),
                                          strParentDirectory, &strErrorMessage)) {
        QMessageBox::critical(this, QStringLiteral("新建项目"), strErrorMessage);
        return;
    }
    // 保存成功后的界面刷新由 projectOpened 信号统一处理
}

void MainWindow::onOpenProject()
{
    const QString strJsonPath = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开项目"), QString(),
        QStringLiteral("攻略项目 (project.json)"));
    if (strJsonPath.isEmpty()) {
        return;
    }
    openProjectPath(strJsonPath);
}

void MainWindow::onOpenRecentProject()
{
    if (QAction* pAction = qobject_cast<QAction*>(sender())) {
        openProjectPath(pAction->data().toString());
    }
}

void MainWindow::onSaveProject()
{
    if (!m_pProjectManager->hasProject()) {
        statusBar()->showMessage(QStringLiteral("当前没有打开的项目"), 3000);
        return;
    }
    // 保存前先同步画布到模型，保证未触发自动保存的编辑也落盘
    syncCanvasToModel();
    QString strErrorMessage;
    if (m_pProjectManager->save(&strErrorMessage)) {
        statusBar()->showMessage(QStringLiteral("已保存"), 3000);
    } else {
        QMessageBox::critical(this, QStringLiteral("保存"), strErrorMessage);
    }
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
    if (!m_pProjectManager->openProject(strJsonPath, &strErrorMessage)) {
        QMessageBox::critical(this, QStringLiteral("打开项目"), strErrorMessage);
    }
}

void MainWindow::refreshRecentProjectsMenu()
{
    m_pRecentProjectsMenu->clear();
    const QStringList recents = Settings::recentProjects();
    if (recents.isEmpty()) {
        QAction* pEmptyAction = m_pRecentProjectsMenu->addAction(QStringLiteral("（无）"));
        pEmptyAction->setEnabled(false);
        return;
    }
    for (const QString& strPath : recents) {
        QAction* pAction = m_pRecentProjectsMenu->addAction(strPath);
        pAction->setData(strPath);
        connect(pAction, &QAction::triggered, this, &MainWindow::onOpenRecentProject);
    }
}

void MainWindow::onProjectOpened()
{
    rebuildProjectTree();
    refreshRecentProjectsMenu();
    updateWindowTitle();
    statusBar()->showMessage(QStringLiteral("已打开项目：%1").arg(m_pProjectManager->projectDirectory()), 5000);
}

void MainWindow::onAutoSavePerformed(bool bOk, const QString& strMessage)
{
    statusBar()->showMessage(strMessage, bOk ? 3000 : 8000);
    updateWindowTitle();
}

void MainWindow::rebuildProjectTree()
{
    m_pProjectTree->clear();
    m_mapNodeKeys.clear();

    if (!m_pProjectManager->hasProject()) {
        return;
    }

    const Project* pProject = m_pProjectManager->project();
    auto* pRootItem = new QTreeWidgetItem(m_pProjectTree);
    pRootItem->setText(0, pProject->strName);
    m_mapNodeKeys.insert(pRootItem, QString());

    for (int nWalkthrough = 0; nWalkthrough < pProject->vecWalkthroughs.size(); ++nWalkthrough) {
        const Walkthrough& rWalkthrough = pProject->vecWalkthroughs.at(nWalkthrough);
        auto* pWalkthroughItem = new QTreeWidgetItem(pRootItem);
        pWalkthroughItem->setText(0, QStringLiteral("%1（%2）")
                                         .arg(rWalkthrough.strTitle,
                                              walkthroughTypeToString(rWalkthrough.eType)));
        m_mapNodeKeys.insert(pWalkthroughItem, QString::number(nWalkthrough));

        for (int nPage = 0; nPage < rWalkthrough.vecPages.size(); ++nPage) {
            auto* pPageItem = new QTreeWidgetItem(pWalkthroughItem);
            pPageItem->setText(0, rWalkthrough.vecPages.at(nPage).strName);
            m_mapNodeKeys.insert(pPageItem, QStringLiteral("%1:%2").arg(nWalkthrough).arg(nPage));
        }
    }
    m_pProjectTree->expandAll();
}

QString MainWindow::selectedPageKey() const
{
    const QList<QTreeWidgetItem*> selected = m_pProjectTree->selectedItems();
    if (selected.isEmpty()) {
        return QString();
    }
    const QString strKey = m_mapNodeKeys.value(selected.first());
    // 仅页面节点（含冒号）才驱动画布
    return strKey.contains(QLatin1Char(':')) ? strKey : QString();
}

Page* MainWindow::currentPage()
{
    const QString strPageKey = selectedPageKey();
    if (strPageKey.isEmpty()) {
        return nullptr;
    }
    const QStringList parts = strPageKey.split(QLatin1Char(':'));
    const int nWalkthroughIndex = parts.at(0).toInt();
    const int nPageIndex = parts.at(1).toInt();
    Project* pProject = m_pProjectManager->project();
    if (!pProject || nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
        return nullptr;
    }
    Walkthrough& rWalkthrough = pProject->vecWalkthroughs[nWalkthroughIndex];
    if (nPageIndex < 0 || nPageIndex >= rWalkthrough.vecPages.size()) {
        return nullptr;
    }
    return &rWalkthrough.vecPages[nPageIndex];
}

void MainWindow::updateCanvasEditor()
{
    Page* pPage = currentPage();
    if (!pPage) {
        m_pScene->clear();
        return;
    }
    m_bSyncingCanvas = true;
    m_pScene->loadPage(*pPage);
    m_bSyncingCanvas = false;
    m_pView->fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
    refreshLayerList();
}

void MainWindow::syncCanvasToModel()
{
    if (m_bSyncingCanvas) {
        return;
    }
    Page* pPage = currentPage();
    if (pPage) {
        m_pScene->syncToModel(pPage);
        m_pProjectManager->setDirty();
        updateWindowTitle();
    }
}

void MainWindow::onCanvasComponentsChanged()
{
    syncCanvasToModel();
    refreshLayerList();
}

void MainWindow::onCanvasSelectionChanged()
{
    // 场景选中变化 → 同步图层面板选中行（guard 防回环）
    if (m_bSyncingCanvas) {
        return;
    }
    m_bSyncingCanvas = true;
    const QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
    for (int nRow = 0; nRow < m_pLayerList->count(); ++nRow) {
        QListWidgetItem* pListItem = m_pLayerList->item(nRow);
        auto* pItem = static_cast<ComponentItem*>(pListItem->data(nComponentItemRole).value<void*>());
        pListItem->setSelected(vecSelected.contains(pItem));
    }
    m_bSyncingCanvas = false;
}

void MainWindow::refreshLayerList()
{
    m_bSyncingCanvas = true;
    m_pLayerList->blockSignals(true);
    m_pLayerList->clear();
    const QVector<ComponentItem*> vecItems = m_pScene->componentItems();
    // 列表行 0 在最底层（zOrder 最小），最后一行在最顶层
    for (const ComponentItem* pItem : vecItems) {
        const Component& rComponent = pItem->component();
        auto* pListItem = new QListWidgetItem(componentDisplayName(rComponent));
        pListItem->setFlags(pListItem->flags() | Qt::ItemIsUserCheckable);
        pListItem->setCheckState(rComponent.bVisible ? Qt::Checked : Qt::Unchecked);
        pListItem->setData(nComponentItemRole, QVariant::fromValue(static_cast<void*>(
            const_cast<ComponentItem*>(pItem))));
        m_pLayerList->addItem(pListItem);
    }
    m_pLayerList->blockSignals(false);
    m_bSyncingCanvas = false;
}

void MainWindow::onAddImageComponent()
{
    const QString strFilePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择图片"), QString(),
        QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (strFilePath.isEmpty()) {
        return;
    }
    Component component;
    component.eType = E_COMPONENT_TYPE_IMAGE;
    component.imageData.strFilePath = strFilePath;
    component.size = QSizeF(300, 200);
    ComponentItem* pNewItem = m_pScene->addComponent(component);
    if (pNewItem) {
        m_pScene->clearSelection();
        pNewItem->setSelected(true);
        m_pView->centerOn(pNewItem);
    }
}

void MainWindow::onAddTextComponent()
{
    bool bOk = false;
    const QString strContent = QInputDialog::getText(
        this, QStringLiteral("插入文本"), QStringLiteral("文本内容："),
        QLineEdit::Normal, QStringLiteral("攻略文本"), &bOk);
    if (!bOk) {
        return;
    }
    Component component;
    component.eType = E_COMPONENT_TYPE_TEXT;
    component.textData.strContent = strContent;
    component.size = QSizeF(300, 60);
    ComponentItem* pNewItem = m_pScene->addComponent(component);
    if (pNewItem) {
        m_pScene->clearSelection();
        pNewItem->setSelected(true);
        m_pView->centerOn(pNewItem);
    }
}

void MainWindow::onAddShapeComponent(int nIndex)
{
    Component component;
    component.eType = E_COMPONENT_TYPE_SHAPE;
    component.shapeData.eShapeType = static_cast<E_SHAPE_TYPE>(nIndex);
    component.size = QSizeF(200, 120);
    ComponentItem* pNewItem = m_pScene->addComponent(component);
    if (pNewItem) {
        m_pScene->clearSelection();
        pNewItem->setSelected(true);
        m_pView->centerOn(pNewItem);
    }
}

void MainWindow::onDeleteSelected()
{
    m_pScene->removeSelectedComponents();
}

void MainWindow::onSelectAllComponents()
{
    for (ComponentItem* pItem : m_pScene->componentItems()) {
        pItem->setSelected(true);
    }
}

QVector<Component> MainWindow::currentComponents()
{
    Page* pPage = currentPage();
    return pPage ? pPage->vecComponents : QVector<Component>();
}

void MainWindow::pushSnapshot(const QString& strText, const QVector<Component>& rBefore,
                              const QVector<Component>& rAfter)
{
    if (rBefore == rAfter) {
        return;
    }
    Page* pPage = currentPage();
    if (!pPage) {
        return;
    }
    m_pUndoStack->push(new PageSnapshotCommand(pPage, rBefore, rAfter, strText, m_pScene));
}

void MainWindow::onComponentEditStarted()
{
    if (m_bInEditTransaction) {
        return;
    }
    m_bInEditTransaction = true;
    m_editBeforeSnapshot = currentComponents();
}

void MainWindow::onComponentEditFinished()
{
    if (!m_bInEditTransaction) {
        return;
    }
    m_bInEditTransaction = false;
    // 编辑期间 geometryChanged 已把模型同步为最新状态
    pushSnapshot(QStringLiteral("移动/缩放组件"), m_editBeforeSnapshot, currentComponents());
}

void MainWindow::onCopy()
{
    m_vecClipboard.clear();
    const QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
    for (const ComponentItem* pItem : vecSelected) {
        m_vecClipboard.append(pItem->component());
    }
    statusBar()->showMessage(QStringLiteral("已复制 %1 个组件").arg(m_vecClipboard.size()), 2000);
}

void MainWindow::onPaste()
{
    if (m_vecClipboard.isEmpty()) {
        return;
    }
    Page* pPage = currentPage();
    if (!pPage) {
        return;
    }
    const QVector<Component> vecBefore = pPage->vecComponents;
    const int nOffset = 20;
    for (const Component& rComponent : m_vecClipboard) {
        Component component = rComponent;
        component.strId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        component.pos += QPointF(nOffset, nOffset);
        component.nZOrder = 0;   // addComponent 自动分配新 zOrder
        m_pScene->addComponent(component);
    }
    // addComponent 已触发同步，模型为粘贴后状态
    pushSnapshot(QStringLiteral("粘贴组件"), vecBefore, currentComponents());
}

void MainWindow::onCut()
{
    if (m_pScene->selectedComponentItems().isEmpty()) {
        return;
    }
    onCopy();
    onDeleteSelected();
}

void MainWindow::onAlignComponents(int nAlign)
{
    QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
    if (vecSelected.size() < 2) {
        statusBar()->showMessage(QStringLiteral("请先选中至少两个组件"), 2000);
        return;
    }

    // 计算选中组件的包围盒
    QRectF boundingBox;
    bool bFirst = true;
    for (const ComponentItem* pItem : vecSelected) {
        const QRectF itemRect(pItem->component().pos, pItem->component().size);
        if (bFirst) {
            boundingBox = itemRect;
            bFirst = false;
        } else {
            boundingBox = boundingBox.united(itemRect);
        }
    }

    const QVector<Component> vecBefore = currentComponents();
    for (ComponentItem* pItem : vecSelected) {
        Component component = pItem->component();
        const qreal dWidth = component.size.width();
        const qreal dHeight = component.size.height();
        switch (nAlign) {
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
    if (vecSelected.size() < 3) {
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
    for (const ComponentItem* pItem : vecSelected) {
        dTotalSize += bHorizontal ? pItem->component().size.width()
                                  : pItem->component().size.height();
    }
    const qreal dGap = (dEnd - dStart - dTotalSize) / (vecSelected.size() - 1);

    const QVector<Component> vecBefore = currentComponents();
    qreal dCursor = dStart;
    for (ComponentItem* pItem : vecSelected) {
        Component component = pItem->component();
        if (bHorizontal) {
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
    if (pHitItem && !pHitItem->isSelected()) {
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
    if (pHitItem) {
        if (pHitItem->component().eType == E_COMPONENT_TYPE_TEXT) {
            menu.addSeparator();
            pEditTextAction = menu.addAction(QStringLiteral("编辑文本…"));
        }
        menu.addSeparator();
        pLockAction = menu.addAction(pHitItem->component().bLocked
                                         ? QStringLiteral("解除锁定")
                                         : QStringLiteral("锁定"));
    }

    QAction* pChosen = menu.exec(QCursor::pos());
    if (!pChosen) {
        return;
    }
    if (pChosen == pCutAction) {
        onCut();
    } else if (pChosen == pCopyAction) {
        onCopy();
    } else if (pChosen == pPasteAction) {
        onPaste();
    } else if (pChosen == pDeleteAction) {
        onDeleteSelected();
    } else if (pChosen == pToTopAction) {
        const QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
        for (ComponentItem* pItem : vecSelected) {
            setItemToTop(pItem, true);
        }
        syncCanvasToModel();
    } else if (pChosen == pToBottomAction) {
        const QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
        for (ComponentItem* pItem : vecSelected) {
            setItemToTop(pItem, false);
        }
        syncCanvasToModel();
    } else if (pChosen == pEditTextAction && pHitItem) {
        pHitItem->editContent();
    } else if (pChosen == pLockAction && pHitItem) {
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

void MainWindow::setItemToTop(ComponentItem* pItem, bool bTop)
{
    int nTargetZ = bTop ? 0 : INT_MAX;
    for (const ComponentItem* pOther : m_pScene->componentItems()) {
        if (pOther == pItem) {
            continue;
        }
        if (bTop) {
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
    for (const ComponentItem* pItem : m_pScene->componentItems()) {
        // 命中检测用组件的矩形范围（不考虑旋转手柄）
        const QRectF itemRect(pItem->component().pos, pItem->component().size);
        if (itemRect.contains(rScenePos)) {
            return const_cast<ComponentItem*>(pItem);
        }
    }
    return nullptr;
}

void MainWindow::moveLayer(int nOffset)
{
    const QList<QListWidgetItem*> selected = m_pLayerList->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const int nRow = m_pLayerList->row(selected.first());
    const int nTarget = nRow + nOffset;
    if (nTarget < 0 || nTarget >= m_pLayerList->count()) {
        return;
    }
    moveLayerTo(nTarget);
}

void MainWindow::moveLayerTo(int nTargetIndex)
{
    const QList<QListWidgetItem*> selected = m_pLayerList->selectedItems();
    if (selected.isEmpty() || m_pLayerList->count() < 2) {
        return;
    }
    const int nSourceIndex = m_pLayerList->row(selected.first());
    if (nSourceIndex == nTargetIndex) {
        return;
    }

    auto* pSourceItem = static_cast<ComponentItem*>(
        m_pLayerList->item(nSourceIndex)->data(nComponentItemRole).value<void*>());
    auto* pTargetItem = static_cast<ComponentItem*>(
        m_pLayerList->item(nTargetIndex)->data(nComponentItemRole).value<void*>());

    // 交换 zOrder 后刷新场景层级
    Component sourceComponent = pSourceItem->component();
    Component targetComponent = pTargetItem->component();
    const int nTemp = sourceComponent.nZOrder;
    sourceComponent.nZOrder = targetComponent.nZOrder;
    targetComponent.nZOrder = nTemp;
    pSourceItem->setComponent(sourceComponent);
    pTargetItem->setComponent(targetComponent);

    m_pScene->sortByZOrder();
    refreshLayerList();
    // 保持选中
    for (int nRow = 0; nRow < m_pLayerList->count(); ++nRow) {
        if (m_pLayerList->item(nRow)->data(nComponentItemRole).value<void*>() == pSourceItem) {
            m_pLayerList->setCurrentRow(nRow);
            break;
        }
    }
    syncCanvasToModel();
}

void MainWindow::onLayerMoveUp()
{
    moveLayer(1);   // 行号增大 = 更上层
}

void MainWindow::onLayerMoveDown()
{
    moveLayer(-1);  // 行号减小 = 更下层
}

void MainWindow::onLayerToTop()
{
    moveLayerTo(m_pLayerList->count() - 1);
}

void MainWindow::onLayerToBottom()
{
    moveLayerTo(0);
}

void MainWindow::onLayerSelectionChanged()
{
    if (m_bSyncingCanvas) {
        return;
    }
    const QList<QListWidgetItem*> selected = m_pLayerList->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    auto* pItem = static_cast<ComponentItem*>(selected.first()->data(nComponentItemRole).value<void*>());
    if (pItem) {
        m_bSyncingCanvas = true;
        m_pScene->clearSelection();
        pItem->setSelected(true);
        m_bSyncingCanvas = false;
        m_pView->centerOn(pItem);
    }
}

void MainWindow::onLayerVisibilityChanged(QListWidgetItem* pListItem)
{
    if (m_bSyncingCanvas) {
        return;
    }
    auto* pItem = static_cast<ComponentItem*>(pListItem->data(nComponentItemRole).value<void*>());
    if (!pItem) {
        return;
    }
    Component component = pItem->component();
    component.bVisible = (pListItem->checkState() == Qt::Checked);
    pItem->setComponent(component);
    pItem->setVisible(component.bVisible);
    syncCanvasToModel();
}

void MainWindow::onProjectTreeSelectionChanged()
{
    updateCanvasEditor();
}

void MainWindow::updateWindowTitle()
{
    if (!m_pProjectManager->hasProject()) {
        setWindowTitle(QStringLiteral("更好的攻略制作器"));
        return;
    }
    const QString strDirtyMark = m_pProjectManager->isDirty() ? QStringLiteral(" *") : QString();
    setWindowTitle(QStringLiteral("%1%2 - 更好的攻略制作器")
                       .arg(m_pProjectManager->project()->strName, strDirtyMark));
}

} // namespace bwm
