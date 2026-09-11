/**
 * @file TextStyleDialog.cpp
 * @author zhangweimu
 * @brief 文本样式对话框实现。
 */
#include "plugin/builtin/TextStyleDialog.h"

#include "ui/FontSelectWidget.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace bwm {

TextStyleDialog::TextStyleDialog(QWidget* pParent, const TextData& rText)
    : QDialog(pParent)
    , m_nInitialAlign(rText.nAlign)
{
    setWindowTitle(QStringLiteral("文本样式"));

    auto* pLayout = new QVBoxLayout(this);

    // ---- 内容与对齐 ----
    auto* pForm = new QFormLayout;
    m_pContentEdit = new QLineEdit(this);
    m_pContentEdit->setText(rText.strContent);
    m_pContentEdit->setPlaceholderText(QStringLiteral("输入文本内容"));
    pForm->addRow(QStringLiteral("内容："), m_pContentEdit);

    m_pAlignCombo = new QComboBox(this);
    // 左对齐项用历史默认值（仅水平左对齐）：插入新文本时与旧版本排版一致。
    // 旧「编辑文本」对话框写入过「左 + 垂直居中」，两者的渲染结果不同，
    // 因此下面对未改动的对齐值做原值保留（见 textData()）。
    m_pAlignCombo->addItem(QStringLiteral("左对齐"), int(Qt::AlignLeft));
    m_pAlignCombo->addItem(QStringLiteral("居中"), int(Qt::AlignHCenter | Qt::AlignVCenter));
    m_pAlignCombo->addItem(QStringLiteral("右对齐"), int(Qt::AlignRight | Qt::AlignVCenter));
    m_pAlignCombo->addItem(QStringLiteral("两端对齐"), int(Qt::AlignJustify | Qt::AlignVCenter));
    // 按「水平对齐」匹配选项，兼容 1（仅左）与 129（左 + 垂直居中）两种历史取值
    int nAlignIndex = 0;
    for (int nIndex = 0; nIndex < m_pAlignCombo->count(); ++nIndex) {
        const int nValue = m_pAlignCombo->itemData(nIndex).toInt();
        if ((nValue & Qt::AlignHorizontal_Mask) == (rText.nAlign & Qt::AlignHorizontal_Mask)) {
            nAlignIndex = nIndex;
            break;
        }
    }
    m_pAlignCombo->setCurrentIndex(nAlignIndex);
    pForm->addRow(QStringLiteral("对齐："), m_pAlignCombo);
    pLayout->addLayout(pForm);

    // ---- 字体与颜色（复用可复用组件：字体族/字号/加粗/颜色/不透明度 + 预览）----
    auto* pFontGroup = new QGroupBox(QStringLiteral("字体与颜色"), this);
    auto* pFontLayout = new QVBoxLayout(pFontGroup);
    m_pFontWidget = new FontSelectWidget(pFontGroup);
    // 空字体族表示「使用默认字体」：用统一的默认值填入，保存后即为显式字体，
    // 显示效果与旧版一致（默认值来源见 ComponentPainter::textDefaultFontFamily）
    m_pFontWidget->setFontFamily(rText.strFontFamily.trimmed().isEmpty() ? textDefaultFontFamily()
                                                                       : rText.strFontFamily);
    m_pFontWidget->setFontSize(rText.nFontSize);
    m_pFontWidget->setBold(rText.bBold);
    m_pFontWidget->setColor(rText.color);
    m_pFontWidget->setOpacityPercent(rText.nOpacityPercent);
    pFontLayout->addWidget(m_pFontWidget);
    pLayout->addWidget(pFontGroup);

    // ---- 按钮 ----
    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    pButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    pButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(pButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    pLayout->addWidget(pButtons);

    // 预览文本跟随内容变化，改样式时能直接看到实际文字的效果
    connect(m_pContentEdit, &QLineEdit::textChanged, this, &TextStyleDialog::onContentChanged);
    onContentChanged();
}

TextData TextStyleDialog::textData() const
{
    TextData result;
    result.strContent = m_pContentEdit->text();
    // 对齐：水平对齐未变时保留原值（历史的 1 / 129 两种「左对齐」都能原样写回），
    // 只有用户真的换了水平对齐方式才写入下拉框的规范值，避免垂直位置无故跳变
    const int nSelectedAlign = m_pAlignCombo->currentData().toInt();
    const bool bAlignUnchanged = (nSelectedAlign & Qt::AlignHorizontal_Mask)
        == (m_nInitialAlign & Qt::AlignHorizontal_Mask);
    result.nAlign = bAlignUnchanged ? m_nInitialAlign : nSelectedAlign;
    result.strFontFamily = m_pFontWidget->fontFamily();
    result.nFontSize = m_pFontWidget->fontSize();
    result.bBold = m_pFontWidget->isBold();
    result.color = m_pFontWidget->color();
    result.nOpacityPercent = m_pFontWidget->opacityPercent();
    return result;
}

void TextStyleDialog::onContentChanged()
{
    // 预览文本跟随内容；过长时截断，避免把对话框撑宽
    constexpr int nMaxPreviewChars = 24;
    QString strPreview = m_pContentEdit->text().trimmed();
    if (strPreview.isEmpty()) {
        strPreview = QStringLiteral("示例文本");
    } else if (strPreview.size() > nMaxPreviewChars) {
        strPreview = strPreview.left(nMaxPreviewChars) + QStringLiteral("…");
    }
    m_pFontWidget->setPreviewText(strPreview);
}

} // namespace bwm
