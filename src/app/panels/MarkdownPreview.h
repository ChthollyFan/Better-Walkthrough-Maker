/**
 * @file MarkdownPreview.h
 * @author zhangweimu
 * @brief Markdown 预览控件：QTextBrowser 子类，渲染 Markdown 并处理图片资源加载。
 *
 * 第二期范围：
 * - setMarkdown() 渲染 CommonMark + GFM
 * - 重写 loadResource() 加载项目内 assets/ 图片（相对路径解析为项目目录绝对路径）
 * - 页面引用 ![[W:P]] 在第三期实现渲染注入，本期仅原样显示为文本占位
 */
#ifndef BWM_APP_PANELS_MARKDOWNPREVIEW_H
#define BWM_APP_PANELS_MARKDOWNPREVIEW_H

#include <QTextBrowser>

namespace bwm {

class ProjectManager;

/**
 * @brief Markdown 预览控件。
 *
 * 基于 QTextBrowser，用 setMarkdown() 渲染。
 * 通过 setProjectDirectory() 设置项目目录，使 ![](assets/xxx.png) 图片引用
 * 能正确解析为项目内绝对路径并加载。
 */
class MarkdownPreview : public QTextBrowser
{
    Q_OBJECT
public:
    explicit MarkdownPreview(QWidget* pParent = nullptr);

    /**
     * @brief 设置项目目录（用于解析 assets/ 相对路径图片）。
     */
    void setProjectDirectory(const QString& strDir);

    /**
     * @brief 设置 Markdown 源码并渲染预览。
     */
    void setMarkdownSource(const QString& strMarkdown);

protected:
    // 重写：解析 assets/ 相对路径，从项目目录加载图片
    QVariant loadResource(int nType, const QUrl& rName) override;

private:
    QString m_strProjectDirectory;   ///< 项目目录（解析图片相对路径用）
};

} // namespace bwm

#endif // BWM_APP_PANELS_MARKDOWNPREVIEW_H
