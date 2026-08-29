/**
 * @file BuiltinTemplateProviders.h
 * @author zhangweimu
 * @brief 内置模板适配器：把 BuiltinTemplates + 项目用户模板
 *        适配为 ITemplateProvider 插件接口。
 */
#ifndef BWM_PLUGIN_BUILTIN_BUILTINTEMPLATEPROVIDERS_H
#define BWM_PLUGIN_BUILTIN_BUILTINTEMPLATEPROVIDERS_H

#include "plugin/ITemplateProvider.h"

namespace bwm {

/**
 * @brief 内置模板 Provider。
 * 合并 BuiltinTemplates（内置模板）与项目用户模板（TemplateManager）。
 */
class BuiltinTemplateProvider : public ITemplateProvider
{
public:
    QString providerId() const override;
    QVector<Template> templates(const QString& rProjectDir) const override;
};

} // namespace bwm

#endif // BWM_PLUGIN_BUILTIN_BUILTINTEMPLATEPROVIDERS_H
