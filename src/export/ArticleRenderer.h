/**
 * @file ArticleRenderer.h
 * @author zhangweimu
 * @brief 文章渲染辅助工具：解析页面引用、加载图片、构造 QTextDocument。
 *
 * 供文章导出 Provider（md / png / pdf）共用，避免重复实现渲染逻辑。
 * 功能：
 * - 解析 ![[W:P]] 页面引用，用 ExportRenderer::renderPage 渲染对应页面为 QImage
 * - 加载 ![](assets/...) 普通图片
 * - 构造一个完整渲染好的 QTextDocument，供导出使用
 */
#ifndef BWM_EXPORT_ARTICLERENDERER_H
#define BWM_EXPORT_ARTICLERENDERER_H

#include <QImage>
#include <QString>

class QTextDocument;

namespace bwm {

struct Article;
struct Project;
struct PluginContext;

/**
 * @brief 文章渲染辅助工具（静态方法集合）。
 */
class ArticleRenderer
{
public:
    /**
     * @brief 构造一个渲染好的 QTextDocument，供导出使用。
     *
     * 解析 ![[W:P]] 页面引用（渲染为图片并注册）、加载 ![](assets/...) 普通图片，
     * 调用 document->setMarkdown 完成渲染。调用方负责管理返回的 document 生命周期。
     *
     * @param rArticle    文章数据
     * @param rProject    所属项目（解析页面引用）
     * @param rContext    插件上下文（项目目录、主题背景色）
     * @param nImageWidth 图片最大显示宽度（像素），用于限制大图
     * @return            堆分配的 QTextDocument（调用方 delete）
     */
    static QTextDocument* buildDocument(const Article& rArticle,
                                         const Project& rProject,
                                         const PluginContext& rContext,
                                         int nImageWidth = 1000);

    /**
     * @brief 渲染指定页面引用为 QImage。
     * @param rProject    所属项目
     * @param nW          攻略索引
     * @param nP          页面索引
     * @param rBackground 背景色
     * @param dScale      渲染倍率
     * @return            渲染结果（失败返回空 QImage）
     */
    static QImage renderPageRef(const Project& rProject, int nW, int nP,
                                 const QColor& rBackground, qreal dScale = 1.0);

    /**
     * @brief 净化文件名：把非法字符替换为下划线。
     */
    static QString sanitizeFileName(const QString& strName);
};

} // namespace bwm

#endif // BWM_EXPORT_ARTICLERENDERER_H
