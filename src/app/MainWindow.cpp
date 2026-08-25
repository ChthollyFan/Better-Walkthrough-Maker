/**
 * @file MainWindow.cpp
 * @author zhangweimu
 * @brief 主窗口实现。
 */
#include "app/MainWindow.h"

#include "core/Project.h"
#include "project/ProjectManager.h"
#include "settings/Settings.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPair>
#include <QPen>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTreeWidget>

namespace bwm {

MainWindow::MainWindow(QWidget* pParent)
    : QMainWindow(pParent)
    , m_pProjectManager(new ProjectManager(this))
    , m_pScene(new QGraphicsScene(this))
{
    setWindowTitle(QStringLiteral("更好的攻略制作器"));
    resize(1200, 800);

    createMenus();
    createCentralWidget();
    createStatusBar();

    connect(m_pProjectManager, &ProjectManager::projectOpened,
            this, &MainWindow::onProjectOpened);
    connect(m_pProjectManager, &ProjectManager::autoSavePerformed,
            this, &MainWindow::onAutoSavePerformed);
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
}

void MainWindow::createCentralWidget()
{
    m_pProjectTree = new QTreeWidget(this);
    m_pProjectTree->setHeaderLabel(QStringLiteral("项目结构"));
    m_pProjectTree->setMinimumWidth(220);
    connect(m_pProjectTree, &QTreeWidget::itemSelectionChanged,
            this, &MainWindow::onProjectTreeSelectionChanged);

    m_pView = new QGraphicsView(m_pScene, this);
    m_pView->setRenderHint(QPainter::Antialiasing);
    m_pView->setBackgroundBrush(QColor(60, 60, 60));
    m_pView->setDragMode(QGraphicsView::RubberBandDrag);

    QSplitter* pSplitter = new QSplitter(Qt::Horizontal, this);
    pSplitter->addWidget(m_pProjectTree);
    pSplitter->addWidget(m_pView);
    pSplitter->setStretchFactor(1, 1);
    pSplitter->setSizes({260, 940});

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
    // 仅页面节点（含冒号）才驱动画布占位
    return strKey.contains(QLatin1Char(':')) ? strKey : QString();
}

void MainWindow::updateCanvasPlaceholder()
{
    m_pScene->clear();

    const QString strPageKey = selectedPageKey();
    if (strPageKey.isEmpty()) {
        return;
    }

    const QStringList parts = strPageKey.split(QLatin1Char(':'));
    const int nWalkthroughIndex = parts.at(0).toInt();
    const int nPageIndex = parts.at(1).toInt();
    const Project* pProject = m_pProjectManager->project();
    if (!pProject || nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
        return;
    }
    const Walkthrough& rWalkthrough = pProject->vecWalkthroughs.at(nWalkthroughIndex);
    if (nPageIndex < 0 || nPageIndex >= rWalkthrough.vecPages.size()) {
        return;
    }
    const Page& rPage = rWalkthrough.vecPages.at(nPageIndex);

    // 最小画布占位：白底页面 + 灰色边框 + 页面名。
    // 这同时验证"场景渲染 = 导出渲染"管线的可行性（M4 用同一场景导出）。
    m_pScene->setSceneRect(0, 0, rPage.size.width(), rPage.size.height());
    m_pScene->addRect(0, 0, rPage.size.width(), rPage.size.height(),
                      QPen(QColor(140, 140, 140)), QBrush(Qt::white));
    auto* pNameText = m_pScene->addSimpleText(rPage.strName);
    pNameText->setBrush(QColor(180, 180, 180));
    pNameText->setPos(8, 8);

    m_pView->fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::onProjectTreeSelectionChanged()
{
    updateCanvasPlaceholder();
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
