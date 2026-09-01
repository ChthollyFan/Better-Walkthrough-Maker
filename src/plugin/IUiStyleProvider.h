/**
 * @file IUiStyleProvider.h
 * @author zhangweimu
 * @brief UI 风格插件接口：第三方可实现此接口注册新的应用 UI 外观。
 *
 * 与 IThemeProvider（画布配色）不同，本接口管的是整个应用窗口/控件的外观
 * （如亚克力、云母等半透明模糊材质，或纯 QSS 模拟风格）。
 *
 * 实现此接口并注册到 PluginHost 后，提供的风格会出现在"主题"菜单的
 * "界面外观"分组中。用户选择后，框架调用 UiStyleManager::setCurrentStyleId
 * 切换并通过 applyStyle 应用到主窗口。
 *
 * 当前为编译时静态注册。未来 PluginHost::loadPlugins() 实现后，
 * 第三方动态库插件亦可提供此接口。
 */
#ifndef BWM_PLUGIN_IUISTYLEPROVIDER_H
#define BWM_PLUGIN_IUISTYLEPROVIDER_H

#include <QString>
#include <QVector>

class QWidget;

namespace bwm {

/**
 * @brief UI 风格描述。
 *
 * 每个风格由一个唯一 id 标识，菜单中显示 strDisplayName。
 * bRequiresNativeApi 标记是否依赖平台原生 API（影响非 Windows 平台可用性）。
 */
struct UiStyleDescriptor {
    QString strId;              ///< 风格唯一标识（如 "acrylic-dark"）
    QString strDisplayName;     ///< 菜单显示名（如 "深色亚克力"）
    bool bRequiresNativeApi = false; ///< 是否依赖平台原生 API
};

/**
 * @brief UI 风格插件接口。
 *
 * 每个实现代表一个 UI 风格来源（内置亚克力风格包、第三方风格包等）。
 */
class IUiStyleProvider
{
public:
    virtual ~IUiStyleProvider() = default;

    /**
     * @brief 风格包唯一标识（如 "builtin-ui"）。
     */
    virtual QString providerId() const = 0;

    /**
     * @brief 提供全部 UI 风格描述列表。
     * @return 风格描述列表
     *
     * 框架在"主题"菜单的"界面外观"分组中合并所有 Provider 的风格。
     */
    virtual QVector<UiStyleDescriptor> styles() const = 0;

    /**
     * @brief 应用指定风格到主窗口。
     * @param strStyleId  styles() 中某项的 strId
     * @param pMainWindow  待应用风格的主窗口
     * @return 是否应用成功（如原生 API 不可用应返回 false，由框架回退到 system）
     */
    virtual bool applyStyle(const QString& strStyleId, QWidget* pMainWindow) const = 0;
};

} // namespace bwm

#endif // BWM_PLUGIN_IUISTYLEPROVIDER_H
