/**
 * @file ProjectTreePanel.cpp
 * @author zhangweimu
 * @brief 项目树面板实现。
 *
 * 逻辑迁移自原 MainWindow 的 rebuildProjectTree / selectedPageKey /
 * selectedNodeKey / selectNodeByKey / onTreeContextMenu / onAddWalkthrough /
 * onAddPage / onRenameNode / onDeleteNode / onSaveAsTemplate /
 * onImportTemplate / onExportTemplate 方法。
 */
#include "app/panels/ProjectTreePanel.h"

#include "project/ProjectManager.h"
#include "plugin/PluginHost.h"
#include "plugin/ITemplateProvider.h"
#include "settings/Settings.h"
#include "template/TemplateManager.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QInputDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUuid>
#include <QVBoxLayout>

namespace bwm {

ProjectTreePanel::ProjectTreePanel(QWidget* pParent, ProjectManager* pProjectManager,
                                   PluginHost* pHost)
    : QWidget(pParent)
    , m_pProjectManager(pProjectManager)
    , m_pHost(pHost)
{
    m_pTree = new QTreeWidget(this);
    m_pTree->setHeaderLabel(QStringLiteral("项目结构"));
    m_pTree->setMinimumWidth(200);
    m_pTree->setContextMenuPolicy(Qt::CustomContextMenu);

    auto* pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(m_pTree);

    connect(m_pTree, &QTreeWidget::itemSelectionChanged,
            this, &ProjectTreePanel::onSelectionChanged);
    connect(m_pTree, &QTreeWidget::customContextMenuRequested,
            this, &ProjectTreePanel::onContextMenu);
}

void ProjectTreePanel::rebuildProjectTree()
{
    m_pTree->clear();
    m_mapNodeKeys.clear();

    if(!m_pProjectManager->hasProject()) {
        return;
    }

    const Project* pProject = m_pProjectManager->project();
    auto* pRootItem = new QTreeWidgetItem(m_pTree);
    pRootItem->setText(0, pProject->strName);
    m_mapNodeKeys.insert(pRootItem, QString());

    for(int nWalkthrough = 0; nWalkthrough < pProject->vecWalkthroughs.size(); ++nWalkthrough) {
        const Walkthrough& rWalkthrough = pProject->vecWalkthroughs.at(nWalkthrough);
        auto* pWalkthroughItem = new QTreeWidgetItem(pRootItem);
        pWalkthroughItem->setText(0, QStringLiteral("%1（%2）")
                                      .arg(rWalkthrough.strTitle,
                                           walkthroughTypeToString(rWalkthrough.eType)));
        m_mapNodeKeys.insert(pWalkthroughItem, QString::number(nWalkthrough));

        for(int nPage = 0; nPage < rWalkthrough.vecPages.size(); ++nPage) {
            auto* pPageItem = new QTreeWidgetItem(pWalkthroughItem);
            pPageItem->setText(0, rWalkthrough.vecPages.at(nPage).strName);
            m_mapNodeKeys.insert(pPageItem, QStringLiteral("%1:%2").arg(nWalkthrough).arg(nPage));
        }
    }
    m_pTree->expandAll();
}

QString ProjectTreePanel::selectedPageKey() const
{
    const QList<QTreeWidgetItem*> selected = m_pTree->selectedItems();
    if(selected.isEmpty()) {
        return QString();
    }
    const QString strKey = m_mapNodeKeys.value(selected.first());
    // 仅页面节点（含冒号）才驱动画布
    return strKey.contains(QLatin1Char(':')) ? strKey : QString();
}

QString ProjectTreePanel::selectedNodeKey() const
{
    const QList<QTreeWidgetItem*> selected = m_pTree->selectedItems();
    if(selected.isEmpty()) {
        return QString();
    }
    return m_mapNodeKeys.value(selected.first());
}

void ProjectTreePanel::selectNodeByKey(const QString& rKey)
{
    for(auto it = m_mapNodeKeys.constBegin(); it != m_mapNodeKeys.constEnd(); ++it) {
        if(it.value() == rKey) {
            m_pTree->setCurrentItem(it.key());
            return;
        }
    }
}

void ProjectTreePanel::onSelectionChanged()
{
    emit pageSelected(selectedPageKey());
}

void ProjectTreePanel::onContextMenu(const QPoint& rPos)
{
    QTreeWidgetItem* pItem = m_pTree->itemAt(rPos);
    if(!pItem) {
        return;
    }
    m_pTree->setCurrentItem(pItem);
    const QString strKey = m_mapNodeKeys.value(pItem);

    QMenu menu(this);
    QAction* pAddWalkthroughAction = nullptr;
    QAction* pAddPageAction = nullptr;
    QAction* pRenameAction = nullptr;
    QAction* pDeleteAction = nullptr;
    if(strKey.isEmpty()) {
        // 项目节点
        pAddWalkthroughAction = menu.addAction(QStringLiteral("新建攻略…"));
    } else if(!strKey.contains(QLatin1Char(':'))) {
        // 攻略节点
        pAddPageAction = menu.addAction(QStringLiteral("新建页面…"));
        menu.addSeparator();
        pRenameAction = menu.addAction(QStringLiteral("重命名攻略…"));
        pDeleteAction = menu.addAction(QStringLiteral("删除攻略…"));
    } else {
        // 页面节点
        pRenameAction = menu.addAction(QStringLiteral("重命名页面…"));
        pDeleteAction = menu.addAction(QStringLiteral("删除页面…"));
    }
    QAction* pChosen = menu.exec(m_pTree->viewport()->mapToGlobal(rPos));
    if(!pChosen) {
        return;
    }
    if(pChosen == pAddWalkthroughAction) {
        onAddWalkthrough();
    } else if(pChosen == pAddPageAction) {
        onAddPage();
    } else if(pChosen == pRenameAction) {
        onRenameNode();
    } else if(pChosen == pDeleteAction) {
        onDeleteNode();
    }
}

void ProjectTreePanel::onAddWalkthrough()
{
    Project* pProject = m_pProjectManager->project();
    if(!pProject) {
        return;
    }
    const QString strProjectDir = m_pProjectManager->projectDirectory();

    // 模板选择对话框：从 PluginHost 获取全部模板 Provider 的模板
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("新建攻略 - 选择模板"));
    dialog.resize(420, 380);
    auto* pDialogLayout = new QVBoxLayout(&dialog);
    auto* pTemplateList = new QListWidget(&dialog);

    QVector<Template> vecTemplates;
    // 遍历所有模板 Provider 合并模板列表
    for(const ITemplateProvider* pProvider : m_pHost->templateProviders()) {
        vecTemplates.append(pProvider->templates(strProjectDir));
    }
    for(const Template& rTemplate : vecTemplates) {
        auto* pItem = new QListWidgetItem(QStringLiteral("%1（%2 页）")
                                              .arg(rTemplate.strName).arg(rTemplate.vecPages.size()));
        pItem->setToolTip(rTemplate.strDescription);
        pItem->setData(Qt::UserRole, rTemplate.strName);
        pTemplateList->addItem(pItem);
    }
    auto* pBlankItem = new QListWidgetItem(QStringLiteral("空白模板（1 页）"));
    pBlankItem->setToolTip(QStringLiteral("从空白页开始，自行排版"));
    pBlankItem->setData(Qt::UserRole, QString());
    pTemplateList->addItem(pBlankItem);
    pTemplateList->setCurrentRow(0);
    pDialogLayout->addWidget(pTemplateList);

    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    pButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("创建"));
    pButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(pButtons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(pButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    pDialogLayout->addWidget(pButtons);

    if(dialog.exec() != QDialog::Accepted || !pTemplateList->currentItem()) {
        return;
    }
    const QString strChosenName = pTemplateList->currentItem()->data(Qt::UserRole).toString();

    Walkthrough walkthrough;
    walkthrough.strTitle = QStringLiteral("攻略 %1").arg(pProject->vecWalkthroughs.size() + 1);
    if(strChosenName.isEmpty()) {
        // 空白模板
        walkthrough.eType = E_WALKTHROUGH_TYPE_COVER;
        Page page;
        page.strName = QStringLiteral("页面 1");
        page.size = Settings::defaultPageSize();
        walkthrough.vecPages.append(page);
    } else {
        // 应用选中模板：复制全部页面，并重新生成组件 id 避免重复
        for(const Template& rTemplate : vecTemplates) {
            if(rTemplate.strName == strChosenName) {
                walkthrough.eType = rTemplate.eType;
                walkthrough.vecPages = rTemplate.vecPages;
                for(Page& rPage : walkthrough.vecPages) {
                    for(Component& rComponent : rPage.vecComponents) {
                        rComponent.strId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                    }
                }
                break;
            }
        }
    }

    pProject->vecWalkthroughs.append(walkthrough);
    m_pProjectManager->setDirty();
    rebuildProjectTree();
    selectNodeByKey(QString::number(pProject->vecWalkthroughs.size() - 1));
    emit projectStructureChanged();
}

void ProjectTreePanel::onAddPage()
{
    Project* pProject = m_pProjectManager->project();
    if(!pProject) {
        return;
    }
    const QString strKey = selectedNodeKey();
    if(strKey.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("新建页面"),
                                 QStringLiteral("请先选中一个攻略"));
        return;
    }
    const int nWalkthroughIndex = strKey.split(QLatin1Char(':')).at(0).toInt();
    if(nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
        return;
    }
    Walkthrough& rWalkthrough = pProject->vecWalkthroughs[nWalkthroughIndex];

    // 新建页面：选择本页画布尺寸
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("新建页面"));
    auto* pFormLayout = new QFormLayout(&dialog);
    auto* pSizeCombo = new QComboBox(&dialog);
    const QList<QPair<QString, QSize>> presets = {
        {QStringLiteral("竖图 1080×1440（推荐）"), QSize(1080, 1440)},
        {QStringLiteral("横图 1920×1080"), QSize(1920, 1080)},
        {QStringLiteral("方形 1080×1080"), QSize(1080, 1080)},
        {QStringLiteral("长图 1080×2400"), QSize(1080, 2400)},
    };
    // 默认选中：该攻略已有页面的尺寸，否则全局默认
    QSize defaultSize = rWalkthrough.vecPages.isEmpty()
        ? Settings::defaultPageSize()
        : rWalkthrough.vecPages.first().size;
    int nDefaultIndex = 0;
    for(int nIndex = 0; nIndex < presets.size(); ++nIndex) {
        pSizeCombo->addItem(presets.at(nIndex).first, presets.at(nIndex).second);
        if(presets.at(nIndex).second == defaultSize) {
            nDefaultIndex = nIndex;
        }
    }
    pSizeCombo->setCurrentIndex(nDefaultIndex);
    pFormLayout->addRow(QStringLiteral("页面尺寸："), pSizeCombo);

    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    pButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("创建"));
    pButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(pButtons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(pButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    pFormLayout->addRow(pButtons);

    if(dialog.exec() != QDialog::Accepted) {
        return;
    }

    Page page;
    page.strName = QStringLiteral("页面 %1").arg(rWalkthrough.vecPages.size() + 1);
    page.size = pSizeCombo->currentData().toSize();
    rWalkthrough.vecPages.append(page);
    m_pProjectManager->setDirty();
    rebuildProjectTree();
    selectNodeByKey(QStringLiteral("%1:%2").arg(nWalkthroughIndex).arg(rWalkthrough.vecPages.size() - 1));
    emit projectStructureChanged();
}

void ProjectTreePanel::onRenameNode()
{
    Project* pProject = m_pProjectManager->project();
    if(!pProject) {
        return;
    }
    const QString strKey = selectedNodeKey();
    QString strOldName;
    if(strKey.isEmpty()) {
        strOldName = pProject->strName;
    } else {
        const QStringList parts = strKey.split(QLatin1Char(':'));
        const int nWalkthroughIndex = parts.at(0).toInt();
        if(nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
            return;
        }
        if(parts.size() == 1) {
            strOldName = pProject->vecWalkthroughs.at(nWalkthroughIndex).strTitle;
        } else {
            const int nPageIndex = parts.at(1).toInt();
            const Walkthrough& rWalkthrough = pProject->vecWalkthroughs.at(nWalkthroughIndex);
            if(nPageIndex < 0 || nPageIndex >= rWalkthrough.vecPages.size()) {
                return;
            }
            strOldName = rWalkthrough.vecPages.at(nPageIndex).strName;
        }
    }
    bool bOk = false;
    const QString strNewName = QInputDialog::getText(this, QStringLiteral("重命名"),
                                                     QStringLiteral("新名称："),
                                                     QLineEdit::Normal, strOldName, &bOk);
    if(!bOk || strNewName.trimmed().isEmpty() || strNewName == strOldName) {
        return;
    }
    if(strKey.isEmpty()) {
        pProject->strName = strNewName.trimmed();
    } else {
        const QStringList parts = strKey.split(QLatin1Char(':'));
        const int nWalkthroughIndex = parts.at(0).toInt();
        if(parts.size() == 1) {
            pProject->vecWalkthroughs[nWalkthroughIndex].strTitle = strNewName.trimmed();
        } else {
            const int nPageIndex = parts.at(1).toInt();
            pProject->vecWalkthroughs[nWalkthroughIndex].vecPages[nPageIndex].strName = strNewName.trimmed();
        }
    }
    m_pProjectManager->setDirty();
    rebuildProjectTree();
    selectNodeByKey(strKey);
    emit projectStructureChanged();
}

void ProjectTreePanel::onDeleteNode()
{
    Project* pProject = m_pProjectManager->project();
    if(!pProject) {
        return;
    }
    const QString strKey = selectedNodeKey();
    if(strKey.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("删除"), QStringLiteral("项目节点不可删除"));
        return;
    }
    const QStringList parts = strKey.split(QLatin1Char(':'));
    const int nWalkthroughIndex = parts.at(0).toInt();
    if(nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
        return;
    }
    QString strConfirm;
    if(parts.size() == 1) {
        strConfirm = QStringLiteral("确定删除攻略「%1」及其全部页面？")
                         .arg(pProject->vecWalkthroughs.at(nWalkthroughIndex).strTitle);
    } else {
        const int nPageIndex = parts.at(1).toInt();
        const Walkthrough& rWalkthrough = pProject->vecWalkthroughs.at(nWalkthroughIndex);
        if(nPageIndex < 0 || nPageIndex >= rWalkthrough.vecPages.size()) {
            return;
        }
        strConfirm = QStringLiteral("确定删除页面「%1」？")
                         .arg(rWalkthrough.vecPages.at(nPageIndex).strName);
    }
    if(QMessageBox::question(this, QStringLiteral("删除"), strConfirm,
                             QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    if(parts.size() == 1) {
        pProject->vecWalkthroughs.removeAt(nWalkthroughIndex);
    } else {
        pProject->vecWalkthroughs[nWalkthroughIndex].vecPages.removeAt(parts.at(1).toInt());
    }
    m_pProjectManager->setDirty();
    rebuildProjectTree();
    emit projectStructureChanged();
}

void ProjectTreePanel::onSaveAsTemplate()
{
    Project* pProject = m_pProjectManager->project();
    if(!pProject) {
        return;
    }
    const QString strKey = selectedNodeKey();
    if(strKey.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("保存为模板"), QStringLiteral("请先选中一个攻略"));
        return;
    }
    const int nWalkthroughIndex = strKey.split(QLatin1Char(':')).at(0).toInt();
    if(nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
        return;
    }
    const Walkthrough& rWalkthrough = pProject->vecWalkthroughs.at(nWalkthroughIndex);
    if(rWalkthrough.vecPages.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("保存为模板"), QStringLiteral("该攻略没有页面，无法保存为模板"));
        return;
    }
    bool bOk = false;
    const QString strTemplateName = QInputDialog::getText(
        this, QStringLiteral("保存为模板"), QStringLiteral("模板名称："),
        QLineEdit::Normal, rWalkthrough.strTitle, &bOk);
    if(!bOk || strTemplateName.trimmed().isEmpty()) {
        return;
    }
    Template t;
    t.strName = strTemplateName.trimmed();
    t.eType = rWalkthrough.eType;
    t.strDescription = QStringLiteral("由攻略「%1」保存").arg(rWalkthrough.strTitle);
    t.vecPages = rWalkthrough.vecPages;
    QString strErrorMessage;
    if(TemplateManager::saveTemplate(t, m_pProjectManager->projectDirectory(), &strErrorMessage)) {
        // 模板保存成功（此处不弹状态栏，由 MainWindow 处理）
    } else {
        QMessageBox::critical(this, QStringLiteral("保存为模板"), strErrorMessage);
    }
}

void ProjectTreePanel::onImportTemplate()
{
    if(!m_pProjectManager->hasProject()) {
        QMessageBox::information(this, QStringLiteral("导入模板"), QStringLiteral("请先打开项目"));
        return;
    }
    const QString strJsonPath = QFileDialog::getOpenFileName(
        this, QStringLiteral("导入模板"), QString(), QStringLiteral("攻略模板 (*.json)"));
    if(strJsonPath.isEmpty()) {
        return;
    }
    QString strErrorMessage;
    if(!TemplateManager::importTemplate(strJsonPath, m_pProjectManager->projectDirectory(),
                                        &strErrorMessage)) {
        QMessageBox::critical(this, QStringLiteral("导入模板"), strErrorMessage);
    }
}

void ProjectTreePanel::onExportTemplate()
{
    Project* pProject = m_pProjectManager->project();
    if(!pProject) {
        return;
    }
    const QString strKey = selectedNodeKey();
    if(strKey.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("导出模板"), QStringLiteral("请先选中一个攻略"));
        return;
    }
    const int nWalkthroughIndex = strKey.split(QLatin1Char(':')).at(0).toInt();
    if(nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
        return;
    }
    const Walkthrough& rWalkthrough = pProject->vecWalkthroughs.at(nWalkthroughIndex);
    if(rWalkthrough.vecPages.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("导出模板"), QStringLiteral("该攻略没有页面，无法导出"));
        return;
    }
    QString strSafeName = rWalkthrough.strTitle;
    strSafeName.replace(QRegularExpression(QStringLiteral(R"([\\/:*?\"<>|])")), QStringLiteral("_"));
    const QString strDestPath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出模板"), strSafeName + QStringLiteral(".json"),
        QStringLiteral("攻略模板 (*.json)"));
    if(strDestPath.isEmpty()) {
        return;
    }
    Template t;
    t.strName = rWalkthrough.strTitle;
    t.eType = rWalkthrough.eType;
    t.vecPages = rWalkthrough.vecPages;
    QString strErrorMessage;
    if(!TemplateSerializer::writeFile(t, strDestPath, &strErrorMessage)) {
        QMessageBox::critical(this, QStringLiteral("导出模板"), strErrorMessage);
    }
}

} // namespace bwm
