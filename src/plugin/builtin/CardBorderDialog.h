/**
 * @file CardBorderDialog.h
 * @author zhangweimu
 * @brief 卡片边框设置对话框：选择边框形状（矩形/正方形/圆形/椭圆）与颜色。
 *
 * 四种形状合并到「卡片边框」一个插入项：插入时弹出本对话框选定形状与颜色，
 * 插入后双击组件或右键「编辑卡片边框…」可再次打开修改。
 * 对话框放在 plugin/builtin/ 层，是因为 core 层的插入流程（StickerComponentProvider）
 * 与 app 层的组件编辑（ComponentItem）都需要调用它。
 *
 * 预览直接复用 ComponentPainter（与画布、导出同一实现），保证所见即所得。
 */
#ifndef BWM_PLUGIN_BUILTIN_CARDBORDERDIALOG_H
#define BWM_PLUGIN_BUILTIN_CARDBORDERDIALOG_H

#include <QDialog>

#include "core/Component.h"

class QComboBox;
class QLabel;
class QPushButton;

namespace bwm {

/**
 * @brief 卡片边框设置对话框。
 */
class CardBorderDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @param pParent   父窗口
     * @param rSticker  当前贴纸数据（形状与颜色作为初始值）
     */
    CardBorderDialog(QWidget* pParent, const StickerData& rSticker);

    /**
     * @brief 用户确认后的贴纸数据（形状 + 颜色）。
     */
    StickerData stickerData() const;

    /**
     * @brief 是否需要把组件尺寸归一为 1:1。
     *
     * 形状为正方形/圆形时为 true：这两者按组件内接正方形绘制，
     * 尺寸归一后选择手柄与图形边界一致，不会出现「框比图形大一圈」。
     */
    bool needsSquareSize() const;

private slots:
    // 选择边框颜色
    void chooseColor();

private:
    // 刷新颜色按钮（色块 + 十六进制文本）
    void updateColorButton();
    // 刷新预览图（复用 ComponentPainter 真实渲染）
    void updatePreview();
    // 当前界面上的贴纸数据
    StickerData currentStickerData() const;

    StickerData m_sticker;                  ///< 编辑中的贴纸数据副本
    QComboBox* m_pShapeCombo = nullptr;     ///< 形状下拉框
    QPushButton* m_pColorButton = nullptr;  ///< 颜色按钮（显示当前色块）
    QLabel* m_pPreviewLabel = nullptr;      ///< 预览
};

} // namespace bwm

#endif // BWM_PLUGIN_BUILTIN_CARDBORDERDIALOG_H
