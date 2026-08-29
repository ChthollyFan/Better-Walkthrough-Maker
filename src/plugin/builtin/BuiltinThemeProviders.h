/**
 * @file BuiltinThemeProviders.h
 * @author zhangweimu
 * @brief 内置主题适配器：把 ThemeManager 管理的内置主题
 *        适配为 IThemeProvider 插件接口。
 */
#ifndef BWM_PLUGIN_BUILTIN_BUILTINTHEMEPROVIDERS_H
#define BWM_PLUGIN_BUILTIN_BUILTINTHEMEPROVIDERS_H

#include "plugin/IThemeProvider.h"

namespace bwm {

/**
 * @brief 内置主题 Provider。
 * 通过 ThemeManager 获取全部内置主题配色。
 */
class BuiltinThemeProvider : public IThemeProvider
{
public:
    QString providerId() const override;
    QVector<Theme> themes() const override;
};

} // namespace bwm

#endif // BWM_PLUGIN_BUILTIN_BUILTINTHEMEPROVIDERS_H
