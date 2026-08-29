/**
 * @file IThemeProvider.h
 * @author zhangweimu
 * @brief 主题插件接口：第三方可实现此接口注册新的主题配色。
 *
 * 实现此接口并注册到 PluginHost 后，对应主题会出现在"主题"菜单中。
 * 当前内置主题由 ThemeManager 静态管理，未来可通过插件扩展。
 */
#ifndef BWM_PLUGIN_ITHEMEPROVIDER_H
#define BWM_PLUGIN_ITHEMEPROVIDER_H

#include <QString>
#include <QVector>

#include "theme/Theme.h"

namespace bwm {

/**
 * @brief 主题插件接口。
 *
 * 每个实现代表一个主题来源（内置主题包、第三方主题包等）。
 */
class IThemeProvider
{
public:
    virtual ~IThemeProvider() = default;

    /**
     * @brief 主题包唯一标识（如 "builtin"）。
     */
    virtual QString providerId() const = 0;

    /**
     * @brief 提供全部主题配色列表。
     * @return 主题列表
     *
     * 框架在"主题"菜单中合并所有 Provider 的主题。
     */
    virtual QVector<Theme> themes() const = 0;
};

} // namespace bwm

#endif // BWM_PLUGIN_ITHEMEPROVIDER_H
