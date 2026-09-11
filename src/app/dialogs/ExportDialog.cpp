/**
 * @file ExportDialog.cpp
 * @author zhangweimu
 * @brief 导出对话框实现。
 *
 * 导出逻辑迁移自原 MainWindow::onExportPng，但格式选择改为
 * 从 PluginHost 动态获取 IExportProvider 列表。
 */
#include "app/dialogs/ExportDialog.h"

#include "app/dialogs/AuthorMarkDialog.h"
#include "core/Article.h"
#include "core/Project.h"
#include "plugin/PluginHost.h"
#include "plugin/IExportProvider.h"
#include "project/ProjectManager.h"
#include "settings/Settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QUrl>
#include <QVariant>
#include <QVBoxLayout>
#include <QVBoxLayout>

namespace bwm {

class ExportDialog::Ui
{
public:
    QRadioButton* pRadioCurrentPage = nullptr;    ///< 导出范围：当前页
    QRadioButton* pRadioWalkthrough = nullptr;    ///< 导出范围：当前攻略全部页
    QComboBox* pFormatCombo = nullptr;            ///< 导出格式下拉框
    QComboBox* pScaleCombo = nullptr;             ///< 分辨率倍率下拉框
    QCheckBox* pAuthorCheck = nullptr;            ///< 添加作者署名
    QPushButton* pAuthorStyleButton = nullptr;    ///< 署名设置…（位置/字体/颜色/字号/透明度）
    QLineEdit* pDirEdit = nullptr;                ///< 导出目录输入框
    QGroupBox* pTargetGroup = nullptr;            ///< 导出范围分组（文章模式下隐藏）
    QGroupBox* pScaleGroup = nullptr;             ///< 倍率分组（文章模式下隐藏，复用 pAuthorCheck 所在行）
};

ExportDialog::ExportDialog(QWidget* pParent, ProjectManager* pProjectManager,
                           PluginHost* pHost, const PluginContext& rContext)
    : QDialog(pParent)
    , m_pUi(new Ui)
    , m_pProjectManager(pProjectManager)
    , m_pHost(pHost)
    , m_context(rContext)
{
    setWindowTitle(QStringLiteral("导出"));
    auto* pFormLayout = new QFormLayout(this);

    // ---- 导出范围 ----
    m_pUi->pTargetGroup = new QGroupBox(QStringLiteral("导出范围"), this);
    auto* pTargetGroup = m_pUi->pTargetGroup;
    auto* pTargetLayout = new QVBoxLayout(pTargetGroup);
    m_pUi->pRadioCurrentPage = new QRadioButton(QStringLiteral("当前页"), pTargetGroup);
    m_pUi->pRadioWalkthrough = new QRadioButton(QStringLiteral("当前攻略全部页"), pTargetGroup);
    m_pUi->pRadioCurrentPage->setChecked(true);
    pTargetLayout->addWidget(m_pUi->pRadioCurrentPage);
    pTargetLayout->addWidget(m_pUi->pRadioWalkthrough);
    pFormLayout->addRow(pTargetGroup);

    // ---- 导出格式（从 PluginHost 动态获取）----
    m_pUi->pFormatCombo = new QComboBox(this);
    for(const IExportProvider* pProvider : m_pHost->exportProviders()) {
        m_pUi->pFormatCombo->addItem(pProvider->displayName(),
                                     QVariant::fromValue(const_cast<IExportProvider*>(pProvider)));
    }
    pFormLayout->addRow(QStringLiteral("导出格式："), m_pUi->pFormatCombo);

    // 范围选"当前页"时格式无意义（一页长图与单页相同），冻结格式选择
    m_pUi->pFormatCombo->setEnabled(false);
    connect(m_pUi->pRadioCurrentPage, &QRadioButton::toggled, m_pUi->pFormatCombo,
            [this](bool bCurrentPage) {
                m_pUi->pFormatCombo->setEnabled(!bCurrentPage);
            });

    // ---- 倍率 ----
    m_pUi->pScaleCombo = new QComboBox(this);
    m_pUi->pScaleCombo->addItem(QStringLiteral("1x（原尺寸）"), 1.0);
    m_pUi->pScaleCombo->addItem(QStringLiteral("2x（推荐，抗平台压缩）"), 2.0);
    m_pUi->pScaleCombo->addItem(QStringLiteral("3x"), 3.0);
    m_pUi->pScaleCombo->setCurrentIndex(1);
    pFormLayout->addRow(QStringLiteral("分辨率倍率："), m_pUi->pScaleCombo);

    // ---- 作者署名选项 ----
    // 复选框控制「是否署名」，右侧「署名设置…」按钮打开水印样式设置对话框
    // （位置 / 字体 / 字号 / 加粗 / 颜色 / 不透明度，见 AuthorMarkDialog）。
    m_pUi->pAuthorCheck = new QCheckBox(this);
    m_pUi->pAuthorCheck->setChecked(!Settings::authorName().trimmed().isEmpty());
    m_pUi->pAuthorStyleButton = new QPushButton(QStringLiteral("署名设置…"), this);
    connect(m_pUi->pAuthorStyleButton, &QPushButton::clicked,
            this, &ExportDialog::onOpenAuthorMarkDialog);
    // 取消勾选后按钮禁用：没有署名就无需调水印样式
    connect(m_pUi->pAuthorCheck, &QCheckBox::toggled, this, [this](bool bChecked) {
        m_pUi->pAuthorStyleButton->setEnabled(bChecked);
    });
    auto* pAuthorRow = new QHBoxLayout;
    pAuthorRow->addWidget(m_pUi->pAuthorCheck);
    pAuthorRow->addWidget(m_pUi->pAuthorStyleButton);
    pFormLayout->addRow(pAuthorRow);
    updateAuthorMarkControls();

    // ---- 导出目录（默认填充上次导出目录）----
    m_pUi->pDirEdit = new QLineEdit(this);
    m_pUi->pDirEdit->setPlaceholderText(QStringLiteral("选择导出目录…"));
    const QString strLastDir = Settings::lastExportDirectory();
    if(!strLastDir.isEmpty()) {
        m_pUi->pDirEdit->setText(strLastDir);
    }
    auto* pBrowseButton = new QPushButton(QStringLiteral("浏览…"), this);
    connect(pBrowseButton, &QPushButton::clicked, this, [this]() {
        const QString strDir = QFileDialog::getExistingDirectory(
            this, QStringLiteral("选择导出目录"));
        if(!strDir.isEmpty()) {
            m_pUi->pDirEdit->setText(strDir);
        }
    });
    auto* pDirRow = new QHBoxLayout;
    pDirRow->addWidget(m_pUi->pDirEdit);
    pDirRow->addWidget(pBrowseButton);
    pFormLayout->addRow(QStringLiteral("导出到："), pDirRow);

    // ---- 提示 ----
    auto* pHintLabel = new QLabel(
        QStringLiteral("提示：长图会把所选范围内的页面纵向拼接；若范围只有一个页面，长图与逐页导出结果相同。"), this);
    pHintLabel->setWordWrap(true);
    pFormLayout->addRow(pHintLabel);

    // ---- 按钮 ----
    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    pButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("导出"));
    pButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(pButtons, &QDialogButtonBox::accepted, this, &ExportDialog::onAccept);
    connect(pButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    pFormLayout->addRow(pButtons);
}

void ExportDialog::setCurrentPageKey(const QString& rKey)
{
    m_strCurrentPageKey = rKey;
}

void ExportDialog::updateAuthorMarkControls()
{
    const QString strAuthorName = Settings::authorName().trimmed();
    if(strAuthorName.isEmpty()) {
        m_pUi->pAuthorCheck->setText(
            QStringLiteral("添加作者署名（请先在 文件→设置 中填写）"));
        m_pUi->pAuthorCheck->setEnabled(false);
    } else {
        // 文案附带当前位置，导出前一眼可见水印会落在哪个角
        const AuthorMarkStyle style = Settings::authorMarkStyle();
        m_pUi->pAuthorCheck->setText(
            QStringLiteral("添加作者署名（by %1 · %2）")
                .arg(strAuthorName, authorMarkPositionDisplayName(style.ePosition)));
        m_pUi->pAuthorCheck->setEnabled(true);
    }
    m_pUi->pAuthorStyleButton->setEnabled(m_pUi->pAuthorCheck->isChecked());
}

void ExportDialog::onOpenAuthorMarkDialog()
{
    AuthorMarkDialog dialog(this, Settings::authorMarkStyle(), Settings::authorName());
    if(dialog.exec() != QDialog::Accepted) {
        return;
    }
    // 署名样式是全局设置：确定后立即持久化，下次导出沿用
    const AuthorMarkStyle style = dialog.style();
    Settings::setAuthorMarkStyle(style);
    // 同步到本次导出上下文：Provider 通过 PluginContext 读取样式（不改插件接口签名）
    m_context.authorMarkStyle = style;
    updateAuthorMarkControls();
}

void ExportDialog::setCurrentArticleKey(const QString& rKey)
{
    m_strCurrentArticleKey = rKey;
    if(rKey.isEmpty()) {
        return;
    }
    // 文章模式：清空格式下拉框，只填入支持文章导出的 Provider
    m_pUi->pFormatCombo->clear();
    for(const IExportProvider* pProvider : m_pHost->exportProviders()) {
        if(pProvider->supportsArticle()) {
            m_pUi->pFormatCombo->addItem(pProvider->displayName(),
                                         QVariant::fromValue(const_cast<IExportProvider*>(pProvider)));
        }
    }
    m_pUi->pFormatCombo->setEnabled(true);
    // 隐藏页面型选项（导出范围、倍率）
    m_pUi->pTargetGroup->hide();
    m_pUi->pScaleCombo->hide();
    // 标题提示为文章导出
    setWindowTitle(QStringLiteral("导出文章"));
}

void ExportDialog::onAccept()
{
    if(!m_pProjectManager->hasProject()) {
        QMessageBox::information(this, QStringLiteral("导出"), QStringLiteral("请先打开项目"));
        return;
    }

    const QString strExportDir = m_pUi->pDirEdit->text().trimmed();
    if(strExportDir.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("导出"), QStringLiteral("请选择导出目录"));
        return;
    }

    // 获取选中的导出 Provider
    auto* pProvider = m_pUi->pFormatCombo->currentData().value<IExportProvider*>();
    if(!pProvider) {
        QMessageBox::critical(this, QStringLiteral("导出"), QStringLiteral("未选择有效的导出格式"));
        return;
    }

    const QString strAuthor = m_pUi->pAuthorCheck->isChecked()
        ? Settings::authorName() : QString();
    // 署名水印样式：从全局设置取最新值同步到本次导出上下文（在署名设置里改完即生效）
    m_context.authorMarkStyle = Settings::authorMarkStyle();

    // ---- 文章导出路径 ----
    if(!m_strCurrentArticleKey.isEmpty()) {
        Project* pProject = m_pProjectManager->project();
        // 文章键格式 "W:A文章索引"
        const QStringList parts = m_strCurrentArticleKey.split(QLatin1Char(':'));
        const int nWalkthroughIndex = parts.at(0).toInt();
        if(nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
            QMessageBox::warning(this, QStringLiteral("导出"), QStringLiteral("无效的文章引用"));
            return;
        }
        const Walkthrough& rWalkthrough = pProject->vecWalkthroughs.at(nWalkthroughIndex);
        const int nArticleIndex = parts.at(1).mid(1).toInt();
        if(nArticleIndex < 0 || nArticleIndex >= rWalkthrough.vecArticles.size()) {
            QMessageBox::warning(this, QStringLiteral("导出"), QStringLiteral("无效的文章引用"));
            return;
        }
        const Article& rArticle = rWalkthrough.vecArticles.at(nArticleIndex);
        const int nExported = pProvider->exportArticle(
            rArticle, *pProject, rArticle.strTitle, strExportDir, strAuthor, m_context, this);
        if(nExported > 0) {
            Settings::setLastExportDirectory(strExportDir);
            accept();
        } else {
            QMessageBox::warning(this, QStringLiteral("导出"), QStringLiteral("导出失败，请检查目录权限"));
        }
        return;
    }

    // ---- 页面导出路径（原有逻辑）----
    Project* pProject = m_pProjectManager->project();
    QVector<Page> vecPages;
    QString strWalkthroughTitle;

    if(m_pUi->pRadioCurrentPage->isChecked()) {
        // 当前页导出
        if(m_strCurrentPageKey.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("导出"),
                                     QStringLiteral("请先在左侧选择要导出的页面"));
            return;
        }
        const int nWalkthroughIndex = m_strCurrentPageKey.split(QLatin1Char(':')).at(0).toInt();
        if(nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
            return;
        }
        const Walkthrough& rWalkthrough = pProject->vecWalkthroughs.at(nWalkthroughIndex);
        const QStringList parts = m_strCurrentPageKey.split(QLatin1Char(':'));
        if(parts.size() < 2) {
            return;
        }
        const int nPageIndex = parts.at(1).toInt();
        if(nPageIndex < 0 || nPageIndex >= rWalkthrough.vecPages.size()) {
            return;
        }
        vecPages.append(rWalkthrough.vecPages.at(nPageIndex));
        strWalkthroughTitle = rWalkthrough.strTitle;
    } else {
        // 全部页导出
        if(m_strCurrentPageKey.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("导出"),
                                     QStringLiteral("请先在左侧选择一个攻略或页面"));
            return;
        }
        const int nWalkthroughIndex = m_strCurrentPageKey.split(QLatin1Char(':')).at(0).toInt();
        if(nWalkthroughIndex < 0 || nWalkthroughIndex >= pProject->vecWalkthroughs.size()) {
            return;
        }
        const Walkthrough& rWalkthrough = pProject->vecWalkthroughs.at(nWalkthroughIndex);
        strWalkthroughTitle = rWalkthrough.strTitle;
        vecPages = rWalkthrough.vecPages;
    }

    if(vecPages.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("导出"), QStringLiteral("没有可导出的页面"));
        return;
    }

    // 执行导出（pProvider / strAuthor 已在函数开头获取）
    const qreal dScale = m_pUi->pScaleCombo->currentData().toDouble();
    const int nExported = pProvider->exportPages(
        vecPages, strWalkthroughTitle, strExportDir, dScale, strAuthor, m_context, this);

    if(nExported > 0) {
        Settings::setLastExportDirectory(strExportDir);
        accept();   // 关闭对话框
    } else {
        QMessageBox::warning(this, QStringLiteral("导出"), QStringLiteral("导出失败，请检查目录权限"));
    }
}

} // namespace bwm
