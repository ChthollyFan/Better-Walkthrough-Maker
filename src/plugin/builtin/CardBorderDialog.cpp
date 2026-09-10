/**
 * @file CardBorderDialog.cpp
 * @author zhangweimu
 * @brief 卡片边框设置对话框实现。
 */
#include "plugin/builtin/CardBorderDialog.h"

#include "core/ComponentPainter.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>

namespace bwm {

namespace {

// 预览画布尺寸（逻辑像素）；正方形/圆形取其中的内接正方形
constexpr int nPreviewWidth = 240;
constexpr int nPreviewHeight = 150;

} // namespace

CardBorderDialog::CardBorderDialog(QWidget* pParent, const StickerData& rSticker)
    : QDialog(pParent)
    , m_sticker(rSticker)
{
    setWindowTitle(QStringLiteral("卡片边框"));
    // 本对话框只服务卡片边框：无论传入什么贴纸类型，一律按卡片边框处理
    m_sticker.eStickerType = E_STICKER_TYPE_CARD_BORDER;

    auto* pForm = new QFormLayout(this);

    m_pShapeCombo = new QComboBox(this);
    m_pShapeCombo->addItem(QStringLiteral("矩形（圆角）"), int(E_CARD_BORDER_SHAPE_RECTANGLE));
    m_pShapeCombo->addItem(QStringLiteral("正方形"), int(E_CARD_BORDER_SHAPE_SQUARE));
    m_pShapeCombo->addItem(QStringLiteral("圆形"), int(E_CARD_BORDER_SHAPE_CIRCLE));
    m_pShapeCombo->addItem(QStringLiteral("椭圆"), int(E_CARD_BORDER_SHAPE_ELLIPSE));
    const int nCurrentIndex = m_pShapeCombo->findData(int(m_sticker.eBorderShape));
    m_pShapeCombo->setCurrentIndex(nCurrentIndex >= 0 ? nCurrentIndex : 0);
    connect(m_pShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updatePreview(); });
    pForm->addRow(QStringLiteral("边框形状："), m_pShapeCombo);

    m_pColorButton = new QPushButton(this);
    connect(m_pColorButton, &QPushButton::clicked, this, &CardBorderDialog::chooseColor);
    pForm->addRow(QStringLiteral("边框颜色："), m_pColorButton);

    m_pPreviewLabel = new QLabel(this);
    m_pPreviewLabel->setAlignment(Qt::AlignCenter);
    m_pPreviewLabel->setMinimumSize(nPreviewWidth, nPreviewHeight);
    pForm->addRow(QStringLiteral("预览："), m_pPreviewLabel);

    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    pButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    pButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(pButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    pForm->addRow(pButtons);

    updateColorButton();
    updatePreview();
}

StickerData CardBorderDialog::currentStickerData() const
{
    StickerData sticker = m_sticker;
    sticker.eBorderShape = static_cast<E_CARD_BORDER_SHAPE>(m_pShapeCombo->currentData().toInt());
    return sticker;
}

StickerData CardBorderDialog::stickerData() const
{
    return currentStickerData();
}

bool CardBorderDialog::needsSquareSize() const
{
    const E_CARD_BORDER_SHAPE eShape = currentStickerData().eBorderShape;
    return eShape == E_CARD_BORDER_SHAPE_SQUARE || eShape == E_CARD_BORDER_SHAPE_CIRCLE;
}

void CardBorderDialog::chooseColor()
{
    const QColor chosen = QColorDialog::getColor(m_sticker.color, this,
                                                 QStringLiteral("选择边框颜色"));
    if (chosen.isValid()) {
        m_sticker.color = chosen;
        updateColorButton();
        updatePreview();
    }
}

void CardBorderDialog::updateColorButton()
{
    // 色块图标 + 十六进制文本，让当前颜色一目了然
    QPixmap swatch(48, 18);
    swatch.fill(m_sticker.color);
    QPainter painter(&swatch);
    painter.setPen(QColor(120, 120, 120));
    painter.drawRect(0, 0, swatch.width() - 1, swatch.height() - 1);
    painter.end();
    m_pColorButton->setIcon(QIcon(swatch));
    m_pColorButton->setText(m_sticker.color.name(QColor::HexRgb).toUpper());
}

void CardBorderDialog::updatePreview()
{
    // 预览直接走 ComponentPainter（画布与导出共用同一实现），保证与最终效果一致
    Component component;
    component.eType = E_COMPONENT_TYPE_STICKER;
    component.stickerData = currentStickerData();
    component.size = QSizeF(nPreviewWidth, nPreviewHeight);

    QImage image(nPreviewWidth, nPreviewHeight, QImage::Format_ARGB32);
    image.fill(palette().color(QPalette::Base));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    ComponentPainter::paint(&painter, component,
                            QRectF(QPointF(0, 0), QSizeF(nPreviewWidth, nPreviewHeight)));
    painter.end();
    m_pPreviewLabel->setPixmap(QPixmap::fromImage(image));
}

} // namespace bwm
