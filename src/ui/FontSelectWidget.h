/**
 * @file FontSelectWidget.h
 * @author zhangweimu
 * @brief 可复用字体设置组件：字体族 / 字号 / 加粗 / 颜色 / 不透明度 + 实时预览。
 *
 * 目前用于「导出 → 署名设置…」；后续凡是需要让用户选字体的地方（例如「插入 → 文本」
 * 的文本样式对话框）都应复用本组件，避免各处重复堆控件、各处默认值不一致。
 *
 * TODO 插入→文本 目前仍只弹 QInputDialog 输入内容，未接入本组件；待后续一并改造。
 */
#ifndef BWM_UI_FONTSELECTWIDGET_H
#define BWM_UI_FONTSELECTWIDGET_H

#include <QColor>
#include <QFont>
#include <QString>
#include <QWidget>

class QCheckBox;
class QFontComboBox;
class QLabel;
class QSlider;
class QSpinBox;
class QToolButton;

namespace bwm {

/**
 * @brief 字体设置组件（表单形式，可直接 addRow 进对话框）。
 *
 * 属性分三组：
 * - 字体：字体族 + 字号 + 加粗（对应 Component 的 TextData）
 * - 颜色：颜色（仅 RGB）+ 不透明度（0~100，单独一列，便于半透明水印）
 * - 预览：按当前字体/颜色/不透明度实时显示示例文本
 */
class FontSelectWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @param pParent  父控件
     */
    explicit FontSelectWidget(QWidget* pParent = nullptr);

    // ---- 字体（字体族 + 字号 + 加粗）----
    QFont font() const;
    void setFont(const QFont& rFont);
    QString fontFamily() const;
    void setFontFamily(const QString& strFamily);
    int fontSize() const;
    void setFontSize(int nSize);
    bool isBold() const;
    void setBold(bool bBold);

    // ---- 颜色 ----
    /**
     * @brief 当前颜色（仅 RGB；不透明度单列，见 opacityPercent）。
     */
    QColor color() const;
    /**
     * @brief 设置颜色；若传入颜色带透明度（alpha < 255），同步换算为不透明度。
     */
    void setColor(const QColor& rColor);
    /**
     * @brief 不透明度（0~100）。
     */
    int opacityPercent() const;
    void setOpacityPercent(int nPercent);

    // ---- 显示控制（不同场景需要的行不一样）----
    // 是否显示颜色与不透明度两行（默认显示）
    void setColorRowsVisible(bool bVisible);
    // 是否显示预览行（默认显示；外部若已有专用预览可关掉）
    void setPreviewVisible(bool bVisible);
    // 预览文本（默认「示例文本」）
    void setPreviewText(const QString& strText);

signals:
    // 字体族/字号/加粗变化
    void fontChanged(const QFont& rFont);
    // 颜色或不透明度变化（返回已应用不透明度的颜色）
    void colorChanged(const QColor& rColor);
    // 任一属性变化（外部刷新自身预览时连这个更省事）
    void styleChanged();

private slots:
    void onFamilyChanged();
    void onSizeChanged();
    void onBoldToggled();
    void onOpacityChanged();
    void onChooseColor();

private:
    // 刷新颜色按钮（色块图标 + 文本）
    void updateColorButton();
    // 刷新预览文本（字体 + 颜色 + 不透明度）
    void updatePreview();
    // 显示/隐藏某个表单行（标签 + 控件）
    void setRowVisible(QLabel* pLabel, QWidget* pField, bool bVisible);

    QLabel* m_pFamilyLabel = nullptr;          ///< 「字体」行标签
    QFontComboBox* m_pFamilyCombo = nullptr;   ///< 字体族下拉框
    QLabel* m_pSizeLabel = nullptr;            ///< 「字号」行标签
    QSpinBox* m_pSizeSpin = nullptr;           ///< 字号输入框
    QLabel* m_pStyleLabel = nullptr;           ///< 「样式」行标签
    QCheckBox* m_pBoldCheck = nullptr;         ///< 加粗开关
    QLabel* m_pColorLabel = nullptr;           ///< 「颜色」行标签
    QToolButton* m_pColorButton = nullptr;     ///< 颜色按钮（色块预览）
    QLabel* m_pOpacityLabel = nullptr;         ///< 「不透明度」行标签
    QWidget* m_pOpacityRow = nullptr;          ///< 不透明度行（滑块 + 数值）
    QSlider* m_pOpacitySlider = nullptr;       ///< 不透明度滑块（0~100）
    QLabel* m_pOpacityValueLabel = nullptr;    ///< 不透明度数值文本
    QLabel* m_pPreviewRowLabel = nullptr;      ///< 「预览」行标签
    QLabel* m_pPreviewLabel = nullptr;         ///< 预览文本

    QColor m_color = QColor(Qt::black);        ///< 当前颜色（仅 RGB）
    int m_nOpacityPercent = 100;               ///< 当前不透明度（0~100）
    QString m_strPreviewText;                  ///< 预览文本
};

} // namespace bwm

#endif // BWM_UI_FONTSELECTWIDGET_H
