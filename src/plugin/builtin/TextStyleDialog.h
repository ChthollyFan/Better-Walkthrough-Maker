/**
 * @file TextStyleDialog.h
 * @author zhangweimu
 * @brief 文本样式对话框：内容 + 对齐 + 字体（复用 FontSelectWidget）。
 *
 * 两个入口共用本对话框，避免两处 UI 不一致：
 * - 插入 → 文本：TextComponentProvider::showInputDialog（插入前设置样式）
 * - 双击组件 / 右键「编辑文本…」：ComponentItem::editTextContent（插入后修改）
 *
 * 放在 plugin/builtin/ 层，是因为 core 层的插入流程与 app 层的组件编辑都要调用它
 * （与 CardBorderDialog 同理）。字体部分直接复用 ui/FontSelectWidget，不重复堆控件。
 */
#ifndef BWM_PLUGIN_BUILTIN_TEXTSTYLEDIALOG_H
#define BWM_PLUGIN_BUILTIN_TEXTSTYLEDIALOG_H

#include <QDialog>

#include "core/Component.h"

class QComboBox;
class QLineEdit;

namespace bwm {

class FontSelectWidget;

/**
 * @brief 文本样式对话框。
 */
class TextStyleDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @param pParent  父窗口
     * @param rText    当前文本数据（内容/对齐/字体/字号/加粗/颜色/不透明度作为初始值）
     */
    TextStyleDialog(QWidget* pParent, const TextData& rText);

    /**
     * @brief 用户确认后的文本数据（内容 + 对齐 + 字体族/字号/加粗/颜色/不透明度）。
     */
    TextData textData() const;

private slots:
    // 内容变化时同步预览文本
    void onContentChanged();

private:
    QLineEdit* m_pContentEdit = nullptr;         ///< 文本内容（单行）
    QComboBox* m_pAlignCombo = nullptr;          ///< 对齐方式
    FontSelectWidget* m_pFontWidget = nullptr;   ///< 字体设置组件（可复用）
    int m_nInitialAlign = Qt::AlignLeft;         ///< 打开时的对齐值（用于避免未改对齐时的数值跳变）
};

} // namespace bwm

#endif // BWM_PLUGIN_BUILTIN_TEXTSTYLEDIALOG_H
