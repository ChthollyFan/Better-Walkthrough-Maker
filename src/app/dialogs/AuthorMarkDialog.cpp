/**
 * @file AuthorMarkDialog.cpp
 * @author zhangweimu
 * @brief 署名设置对话框实现。
 */
#include "app/dialogs/AuthorMarkDialog.h"

#include "export/ExportRenderer.h"
#include "ui/FontSelectWidget.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

#include <array>

namespace bwm {

namespace {

// 预览用的模拟页面尺寸（逻辑像素）；署名按 1x 绘制，便于直接判断字号大小
constexpr int nPreviewPageWidth = 640;
constexpr int nPreviewPageHeight = 400;
// 预览控件显示尺寸（等比缩放，避免对话框被撑得过大）
constexpr int nPreviewDisplayWidth = 480;
constexpr int nPreviewDisplayHeight = 300;

// 四角位置按「左上 → 右上 → 左下 → 右下」列出（与下拉框显示顺序一致）
const std::array<E_AUTHOR_MARK_POSITION, 4> kPositions = {
    E_AUTHOR_MARK_POSITION_TOP_LEFT,
    E_AUTHOR_MARK_POSITION_TOP_RIGHT,
    E_AUTHOR_MARK_POSITION_BOTTOM_LEFT,
    E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT,
};

} // namespace

AuthorMarkDialog::AuthorMarkDialog(QWidget* pParent, const AuthorMarkStyle& rStyle,
                                   const QString& strAuthorName)
    : QDialog(pParent)
    , m_strAuthorName(strAuthorName.trimmed().isEmpty() ? QStringLiteral("作者名")
                                                        : strAuthorName.trimmed())
{
    setWindowTitle(QStringLiteral("署名设置"));

    auto* pLayout = new QVBoxLayout(this);

    // ---- 水印位置 ----
    auto* pPositionForm = new QFormLayout;
    m_pPositionCombo = new QComboBox(this);
    for (const E_AUTHOR_MARK_POSITION ePosition : kPositions) {
        m_pPositionCombo->addItem(authorMarkPositionDisplayName(ePosition), int(ePosition));
    }
    const int nPositionIndex = m_pPositionCombo->findData(int(rStyle.ePosition));
    m_pPositionCombo->setCurrentIndex(nPositionIndex >= 0 ? nPositionIndex : 3);
    pPositionForm->addRow(QStringLiteral("水印位置："), m_pPositionCombo);
    pLayout->addLayout(pPositionForm);

    // ---- 字体与颜色（复用可复用组件）----
    auto* pFontGroup = new QGroupBox(QStringLiteral("字体与颜色"), this);
    auto* pFontLayout = new QVBoxLayout(pFontGroup);
    m_pFontWidget = new FontSelectWidget(pFontGroup);
    // 预览文本用真实署名，字号/颜色在对话框下方的页面预览里一起体现，
    // 因此关掉组件自带的预览行，避免同一信息出现两份。
    m_pFontWidget->setPreviewVisible(false);
    m_pFontWidget->setPreviewText(QStringLiteral("by %1").arg(m_strAuthorName));
    m_pFontWidget->setFontFamily(rStyle.resolvedFontFamily());
    m_pFontWidget->setFontSize(rStyle.nFontSize);
    m_pFontWidget->setBold(rStyle.bBold);
    m_pFontWidget->setColor(rStyle.color);
    m_pFontWidget->setOpacityPercent(rStyle.nOpacityPercent);
    pFontLayout->addWidget(m_pFontWidget);
    pLayout->addWidget(pFontGroup);

    // ---- 预览 ----
    auto* pPreviewGroup = new QGroupBox(QStringLiteral("预览"), this);
    auto* pPreviewLayout = new QVBoxLayout(pPreviewGroup);
    m_pPreviewLabel = new QLabel(pPreviewGroup);
    m_pPreviewLabel->setAlignment(Qt::AlignCenter);
    m_pPreviewLabel->setMinimumSize(nPreviewDisplayWidth, nPreviewDisplayHeight);
    pPreviewLayout->addWidget(m_pPreviewLabel);
    pLayout->addWidget(pPreviewGroup);

    // ---- 按钮 ----
    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    pButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    pButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(pButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    pLayout->addWidget(pButtons);

    // 任一设置变化都刷新预览
    connect(m_pPositionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updatePreview(); });
    connect(m_pFontWidget, &FontSelectWidget::styleChanged, this, &AuthorMarkDialog::updatePreview);

    updatePreview();
}

AuthorMarkStyle AuthorMarkDialog::style() const
{
    AuthorMarkStyle result;
    result.ePosition = static_cast<E_AUTHOR_MARK_POSITION>(
        m_pPositionCombo->currentData().toInt());
    result.strFontFamily = m_pFontWidget->fontFamily();
    result.nFontSize = m_pFontWidget->fontSize();
    result.bBold = m_pFontWidget->isBold();
    result.color = m_pFontWidget->color();
    result.nOpacityPercent = m_pFontWidget->opacityPercent();
    result.clamp();
    return result;
}

void AuthorMarkDialog::updatePreview()
{
    const QImage image = buildPreviewImage();
    m_pPreviewLabel->setPixmap(QPixmap::fromImage(image).scaled(
        nPreviewDisplayWidth, nPreviewDisplayHeight,
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QImage AuthorMarkDialog::buildPreviewImage() const
{
    QImage image(nPreviewPageWidth, nPreviewPageHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    // 模拟页面内容：几条灰色横线，方便判断水印会不会压到正文
    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(224, 224, 224));
    for (int nIndex = 0; nIndex < 5; ++nIndex) {
        const int nY = 48 + nIndex * 58;
        painter.drawRect(QRect(40, nY, nPreviewPageWidth - 80 - (nIndex % 2) * 120, 18));
    }
    painter.end();

    // 用导出同一条绘制路径，保证预览与导出结果一致（1x 绘制，字号所见即所得）
    ExportRenderer::drawAuthorMark(image, m_strAuthorName, style(), 1.0);
    return image;
}

} // namespace bwm
