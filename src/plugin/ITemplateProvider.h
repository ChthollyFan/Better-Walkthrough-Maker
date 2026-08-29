/**
 * @file ITemplateProvider.h
 * @author zhangweimu
 * @brief 模板包插件接口：第三方可实现此接口提供模板集合。
 *
 * 实现此接口并注册到 PluginHost 后，提供的模板会出现在"新建攻略"
 * 的模板选择列表中，与内置模板合并显示。
 *
 * 当前内置模板由 BuiltinTemplates 提供，未来可从动态库插件加载。
 */
#ifndef BWM_PLUGIN_ITEMPLATEPROVIDER_H
#define BWM_PLUGIN_ITEMPLATEPROVIDER_H

#include <QString>
#include <QVector>

#include "core/Template.h"

namespace bwm {

/**
 * @brief 模板包插件接口。
 *
 * 每个实现代表一个模板来源（内置模板包、第三方模板包等）。
 */
class ITemplateProvider
{
public:
    virtual ~ITemplateProvider() = default;

    /**
     * @brief 模板包唯一标识（如 "builtin"）。
     */
    virtual QString providerId() const = 0;

    /**
     * @brief 提供全部模板列表。
     *
     * @param rProjectDir  当前项目目录（某些模板包可能依赖项目路径）
     * @return             模板列表
     *
     * 框架在"新建攻略"对话框中合并所有 Provider 的模板。
     */
    virtual QVector<Template> templates(const QString& rProjectDir) const = 0;
};

} // namespace bwm

#endif // BWM_PLUGIN_ITEMPLATEPROVIDER_H
