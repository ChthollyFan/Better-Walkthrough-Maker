/**
 * @file CardBorderDialog.cpp
 * @author zhangweimu
 * @brief 卡片边框设置对话框实现。
 */
#include "plugin/builtin/CardBorderDialog.h"

#include "core/ComponentPainter.h"
#include "project/AssetStore.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>

namespace bwm {

namespace {

// 预览画布尺寸（逻辑像素）；正方形/圆形取其中的内接正方形
constexpr int nPreviewWidth = 240;
constexpr int nPreviewHeight = 150;

// 可选图片格式（与图片组件、素材库保持一致）
const QString kImageFilter = QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp *.gif)");

// 取景滑块的范围：0~100 映射到取景位置 0.0~1.0
constexpr int nOffsetSliderMax = 100;

} // namespace

CardBorderDialog::CardBorderDialog(QWidget* pParent, const StickerData& rSticker,
                                   const QString& strProjectDirectory)
    : QDialog(pParent)
    , m_sticker(rSticker)
    , m_strProjectDirectory(strProjectDirectory)
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
            this, [this](int) { onShapeChanged(); });
    pForm->addRow(QStringLiteral("边框形状："), m_pShapeCombo);

    m_pColorButton = new QPushButton(this);
    connect(m_pColorButton, &QPushButton::clicked, this, &CardBorderDialog::chooseColor);
    pForm->addRow(QStringLiteral("边框颜色："), m_pColorButton);

    // 图片行：选择 / 清除 + 当前文件名
    auto* pImageRow = new QWidget(this);
    auto* pImageLayout = new QHBoxLayout(pImageRow);
    pImageLayout->setContentsMargins(0, 0, 0, 0);
    m_pChooseImageButton = new QPushButton(QStringLiteral("选择图片…"), pImageRow);
    m_pClearImageButton = new QPushButton(QStringLiteral("清除"), pImageRow);
    m_pImageLabel = new QLabel(pImageRow);
    pImageLayout->addWidget(m_pChooseImageButton);
    pImageLayout->addWidget(m_pClearImageButton);
    pImageLayout->addWidget(m_pImageLabel, 1);
    connect(m_pChooseImageButton, &QPushButton::clicked, this, &CardBorderDialog::chooseImage);
    connect(m_pClearImageButton, &QPushButton::clicked, this, &CardBorderDialog::clearImage);
    pForm->addRow(QStringLiteral("框内图片："), pImageRow);

    // 取景位置：图片按等比覆盖铺满边框，滑块决定露出图片的哪一部分
    m_pOffsetXSlider = new QSlider(Qt::Horizontal, this);
    m_pOffsetYSlider = new QSlider(Qt::Horizontal, this);
    for (QSlider* pSlider : {m_pOffsetXSlider, m_pOffsetYSlider}) {
        pSlider->setRange(0, nOffsetSliderMax);
        pSlider->setSingleStep(1);
        pSlider->setPageStep(10);
        connect(pSlider, &QSlider::valueChanged, this, [this](int) { onOffsetChanged(); });
    }
    pForm->addRow(QStringLiteral("水平位置："), m_pOffsetXSlider);
    pForm->addRow(QStringLiteral("垂直位置："), m_pOffsetYSlider);

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

    // 所有控件创建完成后再给滑块赋初值，并**屏蔽信号**，原因有两个：
    // 1) setValue 会触发 valueChanged → onOffsetChanged → updatePreview，
    //    若在预览控件创建之前触发会访问空指针（表现为点击「插入卡片边框」立即闪退）；
    // 2) onOffsetChanged 会从**两个**滑块读值写回 m_sticker，
    //    给其中一个赋初值时另一个仍是 0，会把该方向的取景位置错误地改成居中。
    {
        const QSignalBlocker blockerX(m_pOffsetXSlider);
        const QSignalBlocker blockerY(m_pOffsetYSlider);
        m_pOffsetXSlider->setValue(qRound(m_sticker.dImageOffsetX * nOffsetSliderMax));
        m_pOffsetYSlider->setValue(qRound(m_sticker.dImageOffsetY * nOffsetSliderMax));
    }

    updateColorButton();
    updateImageRow();
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

void CardBorderDialog::chooseImage()
{
    const QString strFilePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择框内图片"), QString(), kImageFilter);
    if (strFilePath.isEmpty()) {
        return;   // 用户取消
    }

    // 复制进项目 assets/（项目自包含），并以**相对路径**记录，保证项目可整体移动
    QString strErrorMessage;
    const QString strImportedPath = AssetStore::importImage(strFilePath, m_strProjectDirectory,
                                                            &strErrorMessage);
    if (strImportedPath.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("选择图片"), strErrorMessage);
        return;
    }
    m_sticker.strImagePath = AssetStore::toProjectRelative(strImportedPath, m_strProjectDirectory);
    m_imageCache = QImage();   // 换了图片：重新加载预览缓存
    updateImageRow();
    updatePreview();
}

void CardBorderDialog::clearImage()
{
    m_sticker.strImagePath.clear();
    m_sticker.dImageOffsetX = 0.5;   // 取景位置回到居中
    m_sticker.dImageOffsetY = 0.5;
    m_pOffsetXSlider->setValue(nOffsetSliderMax / 2);
    m_pOffsetYSlider->setValue(nOffsetSliderMax / 2);
    m_imageCache = QImage();
    updateImageRow();
    updatePreview();
}

void CardBorderDialog::onShapeChanged()
{
    updatePreview();
}

void CardBorderDialog::onOffsetChanged()
{
    m_sticker.dImageOffsetX = m_pOffsetXSlider->value() / qreal(nOffsetSliderMax);
    m_sticker.dImageOffsetY = m_pOffsetYSlider->value() / qreal(nOffsetSliderMax);
    updatePreview();
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

void CardBorderDialog::updateImageRow()
{
    // 防御：控件未就绪时直接返回（构造过程中可能被信号回调进来）
    if (!m_pClearImageButton || !m_pImageLabel || !m_pOffsetXSlider || !m_pOffsetYSlider) {
        return;
    }
    const bool bHasImage = !m_sticker.strImagePath.isEmpty();
    m_pClearImageButton->setEnabled(bHasImage);
    m_pImageLabel->setText(bHasImage ? QFileInfo(m_sticker.strImagePath).fileName()
                                     : QStringLiteral("（未设置，仅显示边框）"));
    // 没有图片时取景位置无意义，禁用滑块避免误解
    m_pOffsetXSlider->setEnabled(bHasImage);
    m_pOffsetYSlider->setEnabled(bHasImage);
}

void CardBorderDialog::updatePreview()
{
    // 防御：预览控件未创建时直接返回（构造过程中可能被信号回调进来）
    if (!m_pPreviewLabel) {
        return;
    }
    // 预览直接走 ComponentPainter（画布与导出共用同一实现），保证与最终效果一致；
    // 传入项目目录以便解析图片相对路径，并复用同一份图片缓存
    Component component;
    component.eType = E_COMPONENT_TYPE_STICKER;
    component.stickerData = currentStickerData();
    component.size = QSizeF(nPreviewWidth, nPreviewHeight);

    QImage image(nPreviewWidth, nPreviewHeight, QImage::Format_ARGB32);
    image.fill(palette().color(QPalette::Base));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    ComponentPainter::paint(&painter, component,
                            QRectF(QPointF(0, 0), QSizeF(nPreviewWidth, nPreviewHeight)),
                            &m_imageCache, m_strProjectDirectory);
    painter.end();
    m_pPreviewLabel->setPixmap(QPixmap::fromImage(image));
}

} // namespace bwm
