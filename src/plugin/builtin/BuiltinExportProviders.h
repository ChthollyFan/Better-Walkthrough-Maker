/**
 * @file BuiltinExportProviders.h
 * @author zhangweimu
 * @brief 内置导出格式适配器：把现有的 PNG 逐页/长图导出
 *        适配为 IExportProvider 插件接口。
 */
#ifndef BWM_PLUGIN_BUILTIN_BUILTINEXPORTPROVIDERS_H
#define BWM_PLUGIN_BUILTIN_BUILTINEXPORTPROVIDERS_H

#include "plugin/IExportProvider.h"

namespace bwm {

/**
 * @brief PNG 逐页导出 Provider。
 * 将每个页面导出为独立的 PNG 文件，文件名格式：攻略标题_序号.png。
 */
class PngSeparateExportProvider : public IExportProvider
{
public:
    QString formatId() const override;
    QString displayName() const override;
    int exportPages(const QVector<Page>& vecPages,
                    const QString& rWalkthroughTitle,
                    const QString& strDirPath,
                    qreal dScale,
                    const QString& strAuthor,
                    const PluginContext& rContext,
                    QWidget* pParent) const override;
};

/**
 * @brief PNG 长图导出 Provider。
 * 将所有页面纵向拼接为一张长图，可选页间分隔线。
 */
class PngLongImageExportProvider : public IExportProvider
{
public:
    QString formatId() const override;
    QString displayName() const override;
    int exportPages(const QVector<Page>& vecPages,
                    const QString& rWalkthroughTitle,
                    const QString& strDirPath,
                    qreal dScale,
                    const QString& strAuthor,
                    const PluginContext& rContext,
                    QWidget* pParent) const override;
};

} // namespace bwm

#endif // BWM_PLUGIN_BUILTIN_BUILTINEXPORTPROVIDERS_H
