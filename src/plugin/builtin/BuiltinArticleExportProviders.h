/**
 * @file BuiltinArticleExportProviders.h
 * @author zhangweimu
 * @brief 内置文章导出格式 Provider：Markdown / PNG 长图 / PDF。
 *
 * 实现文章攻略的三种导出方式，注册到 PluginHost 后自动出现在导出对话框中
 * （当当前对象是文章时）。
 */
#ifndef BWM_PLUGIN_BUILTIN_BUILTINARTICLEEXPORTPROVIDERS_H
#define BWM_PLUGIN_BUILTIN_BUILTINARTICLEEXPORTPROVIDERS_H

#include "plugin/IExportProvider.h"

namespace bwm {

/**
 * @brief Markdown 文件导出 Provider。
 * 导出原始 .md（保留 ![[W:P]]）+ images/ 目录（页面引用渲染图）+ 兼容版 .md。
 */
class ArticleMarkdownExportProvider : public IExportProvider
{
public:
    QString formatId() const override;
    QString displayName() const override;
    bool supportsArticle() const override;
    int exportArticle(const Article& rArticle,
                       const Project& rProject,
                       const QString& rArticleTitle,
                       const QString& strDirPath,
                       const QString& strAuthor,
                       const PluginContext& rContext,
                       QWidget* pParent) const override;
    // 页面型导出不支持，返回 0
    int exportPages(const QVector<Page>&, const QString&, const QString&,
                    qreal, const QString&, const PluginContext&, QWidget*) const override
    { return 0; }
};

/**
 * @brief PNG 长图导出 Provider。
 * 把文章渲染为单张长图 PNG。
 */
class ArticlePngExportProvider : public IExportProvider
{
public:
    QString formatId() const override;
    QString displayName() const override;
    bool supportsArticle() const override;
    int exportArticle(const Article& rArticle,
                       const Project& rProject,
                       const QString& rArticleTitle,
                       const QString& strDirPath,
                       const QString& strAuthor,
                       const PluginContext& rContext,
                       QWidget* pParent) const override;
    int exportPages(const QVector<Page>&, const QString&, const QString&,
                    qreal, const QString&, const PluginContext&, QWidget*) const override
    { return 0; }
};

/**
 * @brief PDF 导出 Provider。
 * 把文章渲染为分页 PDF。
 */
class ArticlePdfExportProvider : public IExportProvider
{
public:
    QString formatId() const override;
    QString displayName() const override;
    bool supportsArticle() const override;
    int exportArticle(const Article& rArticle,
                       const Project& rProject,
                       const QString& rArticleTitle,
                       const QString& strDirPath,
                       const QString& strAuthor,
                       const PluginContext& rContext,
                       QWidget* pParent) const override;
    int exportPages(const QVector<Page>&, const QString&, const QString&,
                    qreal, const QString&, const PluginContext&, QWidget*) const override
    { return 0; }
};

} // namespace bwm

#endif // BWM_PLUGIN_BUILTIN_BUILTINARTICLEEXPORTPROVIDERS_H
