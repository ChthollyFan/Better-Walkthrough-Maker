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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_projectManager(new ProjectManager(this))
    , m_scene(new QGraphicsScene(this))
{
    setWindowTitle(QStringLiteral("更好的攻略制作器"));
    resize(1200, 800);

    createMenus();
    createCentralWidget();
    createStatusBar();

    connect(m_projectManager, &ProjectManager::projectOpened,
            this, &MainWindow::onProjectOpened);
    connect(m_projectManager, &ProjectManager::autoSavePerformed,
            this, &MainWindow::onAutoSavePerformed);
}

MainWindow::~MainWindow() = default;

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));

    QAction *newAction = fileMenu->addAction(QStringLiteral("新建项目(&N)…"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewProject);

    QAction *openAction = fileMenu->addAction(QStringLiteral("打开项目(&O)…"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenProject);

    m_recentProjectsMenu = fileMenu->addMenu(QStringLiteral("最近项目(&R)"));
    refreshRecentProjectsMenu();

    fileMenu->addSeparator();

    QAction *saveAction = fileMenu->addAction(QStringLiteral("保存(&S)"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveProject);

    QAction *autoSaveAction = fileMenu->addAction(QStringLiteral("自动保存(&A)"));
    autoSaveAction->setCheckable(true);
    autoSaveAction->setChecked(true);
    connect(autoSaveAction, &QAction::toggled, this, &MainWindow::onToggleAutoSave);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction(QStringLiteral("退出(&X)"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::createCentralWidget()
{
    m_projectTree = new QTreeWidget(this);
    m_projectTree->setHeaderLabel(QStringLiteral("项目结构"));
    m_projectTree->setMinimumWidth(220);
    connect(m_projectTree, &QTreeWidget::itemSelectionChanged,
            this, &MainWindow::onProjectTreeSelectionChanged);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setBackgroundBrush(QColor(60, 60, 60));
    m_view->setDragMode(QGraphicsView::RubberBandDrag);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_projectTree);
    splitter->addWidget(m_view);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({260, 940});

    setCentralWidget(splitter);
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

    auto *nameEdit = new QLineEdit(&dialog);
    nameEdit->setPlaceholderText(QStringLiteral("游戏名，如：艾尔登法环"));

    auto *sizeCombo = new QComboBox(&dialog);
    const QSize defaultSize = Settings::defaultPageSize();
    const QList<QPair<QString, QSize>> presets = {
        {QStringLiteral("竖图 1080×1440（默认）"), QSize(1080, 1440)},
        {QStringLiteral("横图 1920×1080"), QSize(1920, 1080)},
        {QStringLiteral("方形 1080×1080"), QSize(1080, 1080)},
        {QStringLiteral("长图 1080×2400"), QSize(1080, 2400)},
    };
    int selectedIndex = 0;
    for (int i = 0; i < presets.size(); ++i) {
        sizeCombo->addItem(presets[i].first, presets[i].second);
        if (presets[i].second == defaultSize)
            selectedIndex = i;
    }
    sizeCombo->setCurrentIndex(selectedIndex);

    auto *form = new QFormLayout(&dialog);
    form->addRow(QStringLiteral("游戏名："), nameEdit);
    form->addRow(QStringLiteral("默认画布尺寸："), sizeCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("创建"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString gameName = nameEdit->text().trimmed();
    if (gameName.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("新建项目"), QStringLiteral("游戏名不能为空"));
        return;
    }

    const QString parentDirectory = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择项目保存位置"));
    if (parentDirectory.isEmpty())
        return;

    QString errorMessage;
    if (!m_projectManager->createProject(gameName, sizeCombo->currentData().toSize(),
                                         parentDirectory, &errorMessage)) {
        QMessageBox::critical(this, QStringLiteral("新建项目"), errorMessage);
        return;
    }
    // 保存成功后的界面刷新由 projectOpened 信号统一处理
}

void MainWindow::onOpenProject()
{
    const QString jsonPath = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开项目"), QString(),
        QStringLiteral("攻略项目 (project.json)"));
    if (jsonPath.isEmpty())
        return;
    openProjectPath(jsonPath);
}

void MainWindow::onOpenRecentProject()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        openProjectPath(action->data().toString());
}

void MainWindow::onSaveProject()
{
    if (!m_projectManager->hasProject()) {
        statusBar()->showMessage(QStringLiteral("当前没有打开的项目"), 3000);
        return;
    }
    QString errorMessage;
    if (m_projectManager->save(&errorMessage))
        statusBar()->showMessage(QStringLiteral("已保存"), 3000);
    else
        QMessageBox::critical(this, QStringLiteral("保存"), errorMessage);
}

void MainWindow::onToggleAutoSave(bool enabled)
{
    m_projectManager->setAutoSaveEnabled(enabled);
    statusBar()->showMessage(enabled ? QStringLiteral("自动保存已开启")
                                     : QStringLiteral("自动保存已关闭"), 3000);
}

void MainWindow::openProjectPath(const QString &jsonPath)
{
    QString errorMessage;
    if (!m_projectManager->openProject(jsonPath, &errorMessage))
        QMessageBox::critical(this, QStringLiteral("打开项目"), errorMessage);
}

void MainWindow::refreshRecentProjectsMenu()
{
    m_recentProjectsMenu->clear();
    const QStringList recents = Settings::recentProjects();
    if (recents.isEmpty()) {
        QAction *emptyAction = m_recentProjectsMenu->addAction(QStringLiteral("（无）"));
        emptyAction->setEnabled(false);
        return;
    }
    for (const QString &path : recents) {
        QAction *action = m_recentProjectsMenu->addAction(path);
        action->setData(path);
        connect(action, &QAction::triggered, this, &MainWindow::onOpenRecentProject);
    }
}

void MainWindow::onProjectOpened()
{
    rebuildProjectTree();
    refreshRecentProjectsMenu();
    updateWindowTitle();
    statusBar()->showMessage(QStringLiteral("已打开项目：%1").arg(m_projectManager->projectDirectory()), 5000);
}

void MainWindow::onAutoSavePerformed(bool ok, const QString &message)
{
    statusBar()->showMessage(message, ok ? 3000 : 8000);
    updateWindowTitle();
}

void MainWindow::rebuildProjectTree()
{
    m_projectTree->clear();
    m_nodeKeys.clear();

    if (!m_projectManager->hasProject())
        return;

    const Project *project = m_projectManager->project();
    auto *rootItem = new QTreeWidgetItem(m_projectTree);
    rootItem->setText(0, project->name);
    m_nodeKeys.insert(rootItem, QString());

    for (int w = 0; w < project->walkthroughs.size(); ++w) {
        const Walkthrough &walkthrough = project->walkthroughs.at(w);
        auto *walkthroughItem = new QTreeWidgetItem(rootItem);
        walkthroughItem->setText(0, QStringLiteral("%1（%2）")
                                     .arg(walkthrough.title, walkthroughTypeToString(walkthrough.type)));
        m_nodeKeys.insert(walkthroughItem, QString::number(w));

        for (int p = 0; p < walkthrough.pages.size(); ++p) {
            auto *pageItem = new QTreeWidgetItem(walkthroughItem);
            pageItem->setText(0, walkthrough.pages.at(p).name);
            m_nodeKeys.insert(pageItem, QStringLiteral("%1:%2").arg(w).arg(p));
        }
    }
    m_projectTree->expandAll();
}

QString MainWindow::selectedPageKey() const
{
    const QList<QTreeWidgetItem *> selected = m_projectTree->selectedItems();
    if (selected.isEmpty())
        return QString();
    const QString key = m_nodeKeys.value(selected.first());
    // 仅页面节点（含冒号）才驱动画布占位
    return key.contains(QLatin1Char(':')) ? key : QString();
}

void MainWindow::updateCanvasPlaceholder()
{
    m_scene->clear();

    const QString pageKey = selectedPageKey();
    if (pageKey.isEmpty())
        return;

    const QStringList parts = pageKey.split(QLatin1Char(':'));
    const int walkthroughIndex = parts.at(0).toInt();
    const int pageIndex = parts.at(1).toInt();
    const Project *project = m_projectManager->project();
    if (!project || walkthroughIndex < 0 || walkthroughIndex >= project->walkthroughs.size())
        return;
    const Walkthrough &walkthrough = project->walkthroughs.at(walkthroughIndex);
    if (pageIndex < 0 || pageIndex >= walkthrough.pages.size())
        return;
    const Page &page = walkthrough.pages.at(pageIndex);

    // 最小画布占位：白底页面 + 灰色边框 + 页面名。
    // 这同时验证"场景渲染 = 导出渲染"管线的可行性（M4 用同一场景导出）。
    m_scene->setSceneRect(0, 0, page.size.width(), page.size.height());
    m_scene->addRect(0, 0, page.size.width(), page.size.height(),
                     QPen(QColor(140, 140, 140)), QBrush(Qt::white));
    auto *nameText = m_scene->addSimpleText(page.name);
    nameText->setBrush(QColor(180, 180, 180));
    nameText->setPos(8, 8);

    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::onProjectTreeSelectionChanged()
{
    updateCanvasPlaceholder();
}

void MainWindow::updateWindowTitle()
{
    if (!m_projectManager->hasProject()) {
        setWindowTitle(QStringLiteral("更好的攻略制作器"));
        return;
    }
    const QString dirtyMark = m_projectManager->isDirty() ? QStringLiteral(" *") : QString();
    setWindowTitle(QStringLiteral("%1%2 - 更好的攻略制作器")
                       .arg(m_projectManager->project()->name, dirtyMark));
}

} // namespace bwm
