/**
 * @file PluginContext.h
 * @author zhangweimu
 * @brief 插件上下文：向插件提供运行时环境信息（主题、当前页面等）。
 *
 * 插件在 createComponent / exportPages 等方法中通过 PluginContext 获取
 * 当前主题配色、画布尺寸等环境信息，无需直接依赖 ThemeManager / Settings。
 * 这样插件只需依赖 plugin/ 目录下的接口头文件，与核心库解耦。
 */
#ifndef BWM_PLUGIN_PLUGINCONTEXT_H
#define BWM_PLUGIN_PLUGINCONTEXT_H

#include <QSize>
#include <QSizeF>
#include <QString>

#include "core/Component.h"
#include "theme/Theme.h"

namespace bwm {

// 前向声明：避免循环包含
struct Page;
class ProjectManager;

/**
 * @brief 插件上下文：在插件方法调用时传入的运行时环境快照。
 *
 * 这是一个值语义的结构体，每次调用时由 PluginHost 构造并传入。
 * 插件通过它获取主题配色、当前页面尺寸等信息，避免直接耦合核心管理器。
 */
struct PluginContext
{
    Theme theme;                    ///< 当前主题配色（主色/辅色/背景色/文字色）
    QSize defaultPageSize;          ///< 全局默认画布尺寸
    QString projectDirectory;       ///< 当前项目目录（素材路径解析用，无项目时为空）
    Page* pCurrentPage = nullptr;   ///< 当前编辑的页面指针（无选中页面时为 nullptr）
};

} // namespace bwm

#endif // BWM_PLUGIN_PLUGINCONTEXT_H
