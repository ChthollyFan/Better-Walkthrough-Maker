/**
 * @file AuthorMarkDialog.h
 * @author zhangweimu
 * @brief 署名设置对话框：水印位置 + 字体（复用 FontSelectWidget）+ 实时预览。
 *
 * 由「导出对话框 → 署名设置…」按钮打开。用户确定后由调用方写回全局设置
 * （Settings::setAuthorMarkStyle）并同步到本次导出的 PluginContext。
 *
 * 预览复用导出同一条绘制路径（ExportRenderer::drawAuthorMark），保证所见即所得。
 */
#ifndef BWM_APP_DIALOGS_AUTHORMARKDIALOG_H
#define BWM_APP_DIALOGS_AUTHORMARKDIALOG_H

#include <QDialog>
#include <QImage>
#include <QString>

#include "core/AuthorMarkStyle.h"

class QComboBox;
class QLabel;

namespace bwm {

class FontSelectWidget;

/**
 * @brief 署名水印设置对话框。
 */
class AuthorMarkDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @param pParent        父窗口
     * @param rStyle         当前样式（作为初始值）
     * @param strAuthorName  作者名（预览水印文本用；为空时用「作者名」占位）
     */
    AuthorMarkDialog(QWidget* pParent, const AuthorMarkStyle& rStyle,
                     const QString& strAuthorName);

    /**
     * @brief 用户确认后的样式（位置 + 字体族/字号/加粗/颜色/不透明度）。
     */
    AuthorMarkStyle style() const;

private slots:
    // 位置或字体属性变化后刷新预览
    void updatePreview();

private:
    // 生成预览图：模拟页面内容 + 按当前样式绘制署名水印
    QImage buildPreviewImage() const;

    QString m_strAuthorName;                     ///< 作者名（预览用）
    QComboBox* m_pPositionCombo = nullptr;       ///< 水印位置下拉框
    FontSelectWidget* m_pFontWidget = nullptr;   ///< 字体设置组件（可复用）
    QLabel* m_pPreviewLabel = nullptr;           ///< 预览图
};

} // namespace bwm

#endif // BWM_APP_DIALOGS_AUTHORMARKDIALOG_H
