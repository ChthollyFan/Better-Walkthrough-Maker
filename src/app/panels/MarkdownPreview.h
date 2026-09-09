/**
 * @file MarkdownPreview.h
 * @author zhangweimu
 * @brief Markdown 预览控件：QTextBrowser 子类，渲染 Markdown 并处理图片资源加载。
 *
 * 功能：
 * - setMarkdown() 渲染 CommonMark + GFM
 * - 重写 loadResource() 加载项目内 assets/ 图片（相对路径解析为项目目录绝对路径）
 * - 页面引用 ![[W:P]] 解析：用 ExportRenderer::renderPage() 渲染对应图文攻略页面为
 *   QImage，注册为 QTextDocument image resource，并替换为标准图片语法显示
 */
#ifndef BWM_APP_PANELS_MARKDOWNPREVIEW_H
#define BWM_APP_PANELS_MARKDOWNPREVIEW_H

#include <QTextBrowser>

namespace bwm {

class ProjectManager;
class Project;
struct Theme;

/**
 * @brief Markdown 预览控件。
 *
 * 基于 QTextBrowser，用 setMarkdown() 渲染。
 * 通过 setProjectDirectory() 设置项目目录，使 ![](assets/xxx.png) 图片引用
 * 能正确解析为项目内绝对路径并加载。
 * 通过 setProject() 设置项目指针，使 ![[W:P]] 页面引用能渲染对应页面。
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
     * @brief 设置项目指针和主题（用于渲染 ![[W:P]] 页面引用）。
     *        Project 指针由调用方保证有效，预览控件不持有所有权。
     */
    void setProject(const Project* pProject, const QColor& rBackgroundColor);

    /**
     * @brief 设置 Markdown 源码并渲染预览。
     *        解析 ![[W:P]] 页面引用，渲染对应页面并注入为图片资源。
     */
    void setMarkdownSource(const QString& strMarkdown);

protected:
    // 重写：解析 assets/ 相对路径与 bwm://page/ 协议，加载对应图片
    QVariant loadResource(int nType, const QUrl& rName) override;
    // 重写：Ctrl+滚轮整体缩放（文字与图片同步缩放）
    void wheelEvent(QWheelEvent* pEvent) override;

private:
    // 用当前源码与缩放因子重新渲染（setMarkdownSource 与缩放共用）
    void renderContent();
    // 渲染指定页面为 QImage 并注册到 document
    QImage renderPageRef(int nWalkthroughIndex, int nPageIndex);

    QString m_strSource;                   ///< 原始 Markdown 源码（缩放时重新渲染用）
    QString m_strProjectDirectory;         ///< 项目目录（解析图片相对路径用）
    const Project* m_pProject = nullptr;   ///< 项目指针（渲染页面引用用）
    QColor m_backgroundColor = Qt::white;  ///< 页面渲染背景色（跟随主题）
    qreal m_dZoom = 1.0;                   ///< 缩放因子（Ctrl+滚轮，0.3~5.0）
    int m_nBaseFontSize = 0;               ///< 基准字号（像素），首次渲染时确定
};

} // namespace bwm

#endif // BWM_APP_PANELS_MARKDOWNPREVIEW_H
