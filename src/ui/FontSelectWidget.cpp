/**
 * @file FontSelectWidget.cpp
 * @author zhangweimu
 * @brief 可复用字体设置组件实现。
 */
#include "ui/FontSelectWidget.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>

namespace bwm {

namespace {

// 字号区间：与「编辑文本」对话框保持一致，避免两处设置范围不同
constexpr int nMinFontSize = 6;
constexpr int nMaxFontSize = 400;
// 默认字号（与 TextData 默认值一致，调用方可覆盖）
constexpr int nDefaultFontSize = 24;
// 不透明度滑块范围（百分比）
constexpr int nMinOpacityPercent = 0;
constexpr int nMaxOpacityPercent = 100;

// 生成颜色按钮的色块图标（描边保证浅色也能看清边界）
QIcon colorSwatchIcon(const QColor& rColor)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(rColor);
    QPainter painter(&pixmap);
    painter.setPen(QPen(QColor(120, 120, 120), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
    return QIcon(pixmap);
}

} // namespace

FontSelectWidget::FontSelectWidget(QWidget* pParent)
    : QWidget(pParent)
    , m_strPreviewText(QStringLiteral("示例文本"))
{
    auto* pForm = new QFormLayout(this);
    pForm->setContentsMargins(0, 0, 0, 0);

    // ---- 字体族 ----
    m_pFamilyLabel = new QLabel(QStringLiteral("字体："), this);
    m_pFamilyCombo = new QFontComboBox(this);
    pForm->addRow(m_pFamilyLabel, m_pFamilyCombo);

    // ---- 字号 ----
    m_pSizeLabel = new QLabel(QStringLiteral("字号："), this);
    m_pSizeSpin = new QSpinBox(this);
    m_pSizeSpin->setRange(nMinFontSize, nMaxFontSize);
    m_pSizeSpin->setValue(nDefaultFontSize);
    m_pSizeSpin->setSuffix(QStringLiteral(" px"));
    pForm->addRow(m_pSizeLabel, m_pSizeSpin);

    // ---- 加粗 ----
    m_pStyleLabel = new QLabel(QStringLiteral("样式："), this);
    m_pBoldCheck = new QCheckBox(QStringLiteral("加粗"), this);
    pForm->addRow(m_pStyleLabel, m_pBoldCheck);

    // ---- 颜色 ----
    m_pColorLabel = new QLabel(QStringLiteral("颜色："), this);
    m_pColorButton = new QToolButton(this);
    m_pColorButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_pColorButton->setIconSize(QSize(16, 16));
    pForm->addRow(m_pColorLabel, m_pColorButton);

    // ---- 不透明度（与颜色分列，避免依赖颜色对话框的 alpha 通道）----
    m_pOpacityLabel = new QLabel(QStringLiteral("不透明度："), this);
    m_pOpacityRow = new QWidget(this);
    auto* pOpacityLayout = new QHBoxLayout(m_pOpacityRow);
    pOpacityLayout->setContentsMargins(0, 0, 0, 0);
    m_pOpacitySlider = new QSlider(Qt::Horizontal, m_pOpacityRow);
    m_pOpacitySlider->setRange(nMinOpacityPercent, nMaxOpacityPercent);
    m_pOpacitySlider->setValue(m_nOpacityPercent);
    m_pOpacityValueLabel = new QLabel(m_pOpacityRow);
    m_pOpacityValueLabel->setMinimumWidth(40);
    pOpacityLayout->addWidget(m_pOpacitySlider);
    pOpacityLayout->addWidget(m_pOpacityValueLabel);
    pForm->addRow(m_pOpacityLabel, m_pOpacityRow);

    // ---- 预览 ----
    m_pPreviewRowLabel = new QLabel(QStringLiteral("预览："), this);
    m_pPreviewLabel = new QLabel(this);
    m_pPreviewLabel->setAlignment(Qt::AlignCenter);
    m_pPreviewLabel->setFrameShape(QFrame::StyledPanel);
    m_pPreviewLabel->setMinimumHeight(44);
    pForm->addRow(m_pPreviewRowLabel, m_pPreviewLabel);

    updateColorButton();
    m_pOpacityValueLabel->setText(QStringLiteral("%1%").arg(m_nOpacityPercent));
    updatePreview();

    // ---- 信号连接（控件 → 数据 → 外部通知）----
    connect(m_pFamilyCombo, &QFontComboBox::currentFontChanged,
            this, [this](const QFont&) { onFamilyChanged(); });
    connect(m_pSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { onSizeChanged(); });
    connect(m_pBoldCheck, &QCheckBox::toggled, this, [this](bool) { onBoldToggled(); });
    connect(m_pOpacitySlider, &QSlider::valueChanged,
            this, [this](int) { onOpacityChanged(); });
    connect(m_pColorButton, &QToolButton::clicked, this, &FontSelectWidget::onChooseColor);
}

QFont FontSelectWidget::font() const
{
    QFont result(m_pFamilyCombo->currentFont().family());
    result.setPixelSize(m_pSizeSpin->value());
    result.setBold(m_pBoldCheck->isChecked());
    return result;
}

void FontSelectWidget::setFont(const QFont& rFont)
{
    setFontFamily(rFont.family());
    if (rFont.pixelSize() > 0) {
        setFontSize(rFont.pixelSize());
    }
    setBold(rFont.bold());
}

QString FontSelectWidget::fontFamily() const
{
    return m_pFamilyCombo->currentFont().family();
}

void FontSelectWidget::setFontFamily(const QString& strFamily)
{
    if (strFamily.trimmed().isEmpty()) {
        return;   // 空字体族由调用方决定回退值，组件本身不猜
    }
    if (m_pFamilyCombo->currentFont().family() == strFamily) {
        return;
    }
    const QSignalBlocker blocker(m_pFamilyCombo);   // 避免 setter 反过来触发信号
    m_pFamilyCombo->setCurrentFont(QFont(strFamily));
    updatePreview();
}

int FontSelectWidget::fontSize() const
{
    return m_pSizeSpin->value();
}

void FontSelectWidget::setFontSize(int nSize)
{
    const int nClamped = qBound(nMinFontSize, nSize, nMaxFontSize);
    if (m_pSizeSpin->value() == nClamped) {
        return;
    }
    const QSignalBlocker blocker(m_pSizeSpin);
    m_pSizeSpin->setValue(nClamped);
    updatePreview();
}

bool FontSelectWidget::isBold() const
{
    return m_pBoldCheck->isChecked();
}

void FontSelectWidget::setBold(bool bBold)
{
    if (m_pBoldCheck->isChecked() == bBold) {
        return;
    }
    const QSignalBlocker blocker(m_pBoldCheck);
    m_pBoldCheck->setChecked(bBold);
    updatePreview();
}

QColor FontSelectWidget::color() const
{
    return m_color;
}

void FontSelectWidget::setColor(const QColor& rColor)
{
    if (!rColor.isValid()) {
        return;
    }
    m_color = QColor(rColor.red(), rColor.green(), rColor.blue());
    // 传入颜色带透明度时（例如旧配置里的 alpha）同步换算到不透明度滑块；
    // 这里直接改内部状态，避免调用 setOpacityPercent 造成信号重复发出。
    if (rColor.alpha() < 255) {
        m_nOpacityPercent = qBound(nMinOpacityPercent, qRound(rColor.alphaF() * 100.0),
                                   nMaxOpacityPercent);
        const QSignalBlocker blocker(m_pOpacitySlider);
        m_pOpacitySlider->setValue(m_nOpacityPercent);
        m_pOpacityValueLabel->setText(QStringLiteral("%1%").arg(m_nOpacityPercent));
    }
    updateColorButton();
    updatePreview();
    emit colorChanged(m_color);
    emit styleChanged();
}

int FontSelectWidget::opacityPercent() const
{
    return m_nOpacityPercent;
}

void FontSelectWidget::setOpacityPercent(int nPercent)
{
    const int nClamped = qBound(nMinOpacityPercent, nPercent, nMaxOpacityPercent);
    if (m_nOpacityPercent == nClamped) {
        return;
    }
    m_nOpacityPercent = nClamped;
    const QSignalBlocker blocker(m_pOpacitySlider);
    m_pOpacitySlider->setValue(nClamped);
    m_pOpacityValueLabel->setText(QStringLiteral("%1%").arg(nClamped));
    updatePreview();
    emit colorChanged(m_color);
    emit styleChanged();
}

void FontSelectWidget::setColorRowsVisible(bool bVisible)
{
    setRowVisible(m_pColorLabel, m_pColorButton, bVisible);
    setRowVisible(m_pOpacityLabel, m_pOpacityRow, bVisible);
}

void FontSelectWidget::setPreviewVisible(bool bVisible)
{
    setRowVisible(m_pPreviewRowLabel, m_pPreviewLabel, bVisible);
}

void FontSelectWidget::setPreviewText(const QString& strText)
{
    m_strPreviewText = strText;
    updatePreview();
}

void FontSelectWidget::onFamilyChanged()
{
    updatePreview();
    emit fontChanged(font());
    emit styleChanged();
}

void FontSelectWidget::onSizeChanged()
{
    updatePreview();
    emit fontChanged(font());
    emit styleChanged();
}

void FontSelectWidget::onBoldToggled()
{
    updatePreview();
    emit fontChanged(font());
    emit styleChanged();
}

void FontSelectWidget::onOpacityChanged()
{
    m_nOpacityPercent = m_pOpacitySlider->value();
    m_pOpacityValueLabel->setText(QStringLiteral("%1%").arg(m_nOpacityPercent));
    updatePreview();
    emit colorChanged(m_color);
    emit styleChanged();
}

void FontSelectWidget::onChooseColor()
{
    // 不显示 alpha 通道：透明度由独立滑块负责，避免两个入口互相打架
    const QColor chosen = QColorDialog::getColor(m_color, this, QStringLiteral("选择颜色"));
    if (chosen.isValid()) {
        setColor(chosen);
    }
}

void FontSelectWidget::updateColorButton()
{
    m_pColorButton->setText(QStringLiteral("选择颜色"));
    m_pColorButton->setIcon(colorSwatchIcon(m_color));
}

void FontSelectWidget::updatePreview()
{
    m_pPreviewLabel->setText(m_strPreviewText);
    m_pPreviewLabel->setFont(font());
    // 预览颜色用 rgba：把不透明度一起体现出来（与导出水印的绘制效果一致）
    m_pPreviewLabel->setStyleSheet(
        QStringLiteral("color: rgba(%1, %2, %3, %4);")
            .arg(m_color.red())
            .arg(m_color.green())
            .arg(m_color.blue())
            .arg(m_nOpacityPercent / 100.0, 0, 'f', 3));
}

void FontSelectWidget::setRowVisible(QLabel* pLabel, QWidget* pField, bool bVisible)
{
    if (pLabel) {
        pLabel->setVisible(bVisible);
    }
    if (pField) {
        pField->setVisible(bVisible);
    }
}

} // namespace bwm
