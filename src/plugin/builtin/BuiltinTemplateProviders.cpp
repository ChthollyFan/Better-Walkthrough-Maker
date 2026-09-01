/**
 * @file BuiltinTemplateProviders.cpp
 * @author zhangweimu
 * @brief 内置模板适配器实现。
 */
#include "plugin/builtin/BuiltinTemplateProviders.h"

#include "template/TemplateManager.h"

namespace bwm {

QString BuiltinTemplateProvider::providerId() const
{
    return QStringLiteral("builtin");
}

QVector<Template> BuiltinTemplateProvider::templates(const QString& rProjectDir) const
{
    // TemplateManager::allTemplates 已合并内置模板与项目用户模板
    return TemplateManager::allTemplates(rProjectDir);
}

} // namespace bwm
