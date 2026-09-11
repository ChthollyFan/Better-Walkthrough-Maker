/**
 * @file CardBorderDialog.h
 * @author zhangweimu
 * @brief 卡片边框设置对话框：边框形状、颜色，以及框内展示的图片与取景位置。
 *
 * 四种形状合并到「卡片边框」一个插入项：插入时弹出本对话框，插入后双击组件
 * 或右键「编辑卡片边框…」可再次打开修改。
 * 对话框放在 plugin/builtin/ 层，是因为 core 层的插入流程（StickerComponentProvider）
 * 与 app 层的组件编辑（ComponentItem）都需要调用它。
 *
 * 预览直接复用 ComponentPainter（与画布、导出同一实现），保证所见即所得。
 */
#ifndef BWM_PLUGIN_BUILTIN_CARDBORDERDIALOG_H
#define BWM_PLUGIN_BUILTIN_CARDBORDERDIALOG_H

#include <QDialog>
#include <QImage>
#include <QString>

#include "core/Component.h"

class QComboBox;
class QLabel;
class QPushButton;
class QSlider;

namespace bwm {

/**
 * @brief 卡片边框设置对话框。
 */
class CardBorderDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @param pParent             父窗口
     * @param rSticker            当前贴纸数据（形状、颜色、图片与取景位置作为初始值）
     * @param strProjectDirectory 项目目录：选图时复制进 assets/，并把图片存为项目内相对路径
     */
    CardBorderDialog(QWidget* pParent, const StickerData& rSticker,
                     const QString& strProjectDirectory);

    /**
     * @brief 用户确认后的贴纸数据（形状 + 颜色 + 图片 + 取景位置）。
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
    // 选择框内图片（复制进项目 assets/，记录相对路径）
    void chooseImage();
    // 清除框内图片（只清引用，素材文件保留在素材库）
    void clearImage();
    // 形状或取景位置变化后同步数据并刷新预览
    void onShapeChanged();
    void onOffsetChanged();

private:
    // 刷新颜色按钮（色块 + 十六进制文本）
    void updateColorButton();
    // 刷新图片行（文件名与按钮可用状态）
    void updateImageRow();
    // 刷新预览图（复用 ComponentPainter 真实渲染）
    void updatePreview();
    // 当前界面上的贴纸数据
    StickerData currentStickerData() const;

    StickerData m_sticker;                  ///< 编辑中的贴纸数据副本
    QString m_strProjectDirectory;          ///< 项目目录（图片导入与路径解析）
    QComboBox* m_pShapeCombo = nullptr;     ///< 形状下拉框
    QPushButton* m_pColorButton = nullptr;  ///< 颜色按钮（显示当前色块）
    QPushButton* m_pChooseImageButton = nullptr;   ///< 选择图片
    QPushButton* m_pClearImageButton = nullptr;    ///< 清除图片
    QLabel* m_pImageLabel = nullptr;        ///< 当前图片文件名
    QSlider* m_pOffsetXSlider = nullptr;    ///< 水平取景位置
    QSlider* m_pOffsetYSlider = nullptr;    ///< 垂直取景位置
    QLabel* m_pPreviewLabel = nullptr;      ///< 预览
    QImage m_imageCache;                    ///< 预览用图片缓存（避免拖动滑块时反复读盘）
};

} // namespace bwm

#endif // BWM_PLUGIN_BUILTIN_CARDBORDERDIALOG_H
