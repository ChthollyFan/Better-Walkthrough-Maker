/**
 * @file UiStyle.h
 * @author zhangweimu
 * @brief 应用 UI 外观管理：与画布配色主题(Theme)平行的窗口/控件外观层。
 *
 * UiStyleManager 负责：
 * - 当前 UI 风格 id 的持久化（Settings 键 "ui/style"）
 * - 合并所有已注册 IUiStyleProvider 提供的风格列表
 * - 将当前风格应用到主窗口（查找对应 Provider 调用 applyStyle）
 *
 * 与 ThemeManager（静态、内置硬编码）不同，UiStyleManager 支持通过
 * PluginHost 动态注册 Provider，便于第三方扩展。
 */
#ifndef BWM_UI_UISTYLE_H
#define BWM_UI_UISTYLE_H

#include <QString>
#include <QVector>

#include "plugin/IUiStyleProvider.h"

class QWidget;

namespace bwm {

/**
 * @brief 应用 UI 外观管理器（静态接口）。
 *
 * 用法：
 * 1. 应用启动时由 BuiltinPluginRegistrar 调用 registerProvider 注册内置 Provider。
 * 2. MainWindow 构造时调用 applyCurrentStyle(this) 应用持久化的风格。
 * 3. 用户在菜单切换风格时调用 setCurrentStyleId + applyCurrentStyle。
 */
class UiStyleManager
{
public:
    // 内置风格 id 常量
    static const QString kSystemId;        ///< "system" —— 系统默认外观
    static const QString kAcrylicDarkId;   ///< "acrylic-dark"
    static const QString kAcrylicLightId;  ///< "acrylic-light"

    /**
     * @brief 当前 UI 风格 id（持久化于 Settings，默认 acrylic-dark）。
     */
    static QString currentStyleId();
    static void setCurrentStyleId(const QString& rId);

    /**
     * @brief 合并所有已注册 Provider 提供的风格描述列表。
     *
     * 仅包含各 Provider 提供的风格（如亚克力）。system 风格不在此列表中——
     * 它仅作为运行时 fallback（非 Windows 平台选了亚克力时降级禁用），不暴露给用户。
     */
    static QVector<UiStyleDescriptor> availableStyles();

    /**
     * @brief 确保当前风格 id 在可用列表内。
     *
     * 若持久化的当前风格不在 availableStyles() 里（如用户之前选过 system，
     * 或 Provider 卸载后风格失效），自动回退到第一个可用风格并持久化。
     * 应在菜单构建前调用，避免菜单出现"无选中项"。
     */
    static void ensureCurrentStyleAvailable();

    /**
     * @brief 注册一个 UI 风格 Provider。
     * @note  Provider 所有权归调用方，UiStyleManager 仅持有指针。
     */
    static void registerProvider(IUiStyleProvider* pProvider);

    /**
     * @brief 应用当前风格到主窗口。
     * @param pMainWindow  待应用风格的主窗口
     * @return 是否成功；失败时自动回退到 "system" 并返回 false
     *
     * 遍历所有已注册 Provider，找到能处理当前 styleId 的那个调用其 applyStyle。
     * 若所有 Provider 都返回 false，则禁用所有原生效果（回退 system）。
     */
    static bool applyCurrentStyle(QWidget* pMainWindow);

private:
    // 查找能处理指定 styleId 的 Provider 并调用 applyStyle
    static bool applyStyleById(const QString& rId, QWidget* pMainWindow);
};

} // namespace bwm

#endif // BWM_UI_UISTYLE_H
