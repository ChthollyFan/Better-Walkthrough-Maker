/**
 * @file PluginHost.h
 * @author zhangweimu
 * @brief 插件宿主：所有扩展点的注册中心。
 *
 * PluginHost 是面向插件化的核心模块——它提供一个统一的注册/查询接口，
 * 背后管理五类扩展点的注册表（组件类型、导出格式、面板、模板、主题）。
 *
 * 设计原则（深模块）：
 * - 小接口：对外只有 register 和 providers 两类方法。
 * - 大实现：隐藏注册表的存储、合并、查询逻辑。
 * - 面板和对话框通过 PluginHost 获取能力，不直接持有各 Provider 列表。
 *
 * 当前为编译时静态注册（BuiltinProviders 在应用初始化时注册内置功能）。
 * 预留升级路径：未来在 loadPlugins() 中扫描 plugins/ 目录，
 * 通过 QPluginLoader 动态加载 .dll/.so 并调用其注册方法。
 */
#ifndef BWM_PLUGIN_PLUGINHOST_H
#define BWM_PLUGIN_PLUGINHOST_H

#include <QObject>
#include <QVector>

namespace bwm {

class IComponentProvider;
class IExportProvider;
class IPanelProvider;
class ITemplateProvider;
class IThemeProvider;
class IUiStyleProvider;

/**
 * @brief 插件宿主：管理全部扩展点注册表。
 *
 * 生命周期：由 MainWindow 创建并持有，随 MainWindow 销毁。
 * 各面板/对话框通过 PluginHost 指针查询可用扩展点。
 */
class PluginHost : public QObject
{
    Q_OBJECT
public:
    explicit PluginHost(QObject* pParent = nullptr);
    ~PluginHost() override;

    // ---- 组件类型扩展点 ----

    /**
     * @brief 注册一个组件类型 Provider。
     * @note  Provider 的所有权归调用方（通常为静态对象或 MainWindow），
     *        PluginHost 仅持有指针，不负责销毁。
     */
    void registerComponentProvider(IComponentProvider* pProvider);

    /**
     * @brief 获取全部已注册的组件类型 Provider。
     * @return Provider 指针列表（按注册顺序）
     */
    const QVector<IComponentProvider*>& componentProviders() const;

    // ---- 导出格式扩展点 ----

    void registerExportProvider(IExportProvider* pProvider);
    const QVector<IExportProvider*>& exportProviders() const;

    // ---- 面板扩展点 ----

    void registerPanelProvider(IPanelProvider* pProvider);
    const QVector<IPanelProvider*>& panelProviders() const;

    // ---- 模板扩展点 ----

    void registerTemplateProvider(ITemplateProvider* pProvider);
    const QVector<ITemplateProvider*>& templateProviders() const;

    // ---- 主题扩展点（画布配色）----

    void registerThemeProvider(IThemeProvider* pProvider);
    const QVector<IThemeProvider*>& themeProviders() const;

    // ---- UI 风格扩展点（窗口/控件外观）----

    void registerUiStyleProvider(IUiStyleProvider* pProvider);
    const QVector<IUiStyleProvider*>& uiStyleProviders() const;

    // TODO: 未来在此添加 loadPlugins() 方法，扫描 plugins/ 目录
    //       通过 QPluginLoader 动态加载动态库插件并调用其注册方法。
    //       void loadPlugins(const QString& rPluginsDir);

private:
    QVector<IComponentProvider*>  m_vecComponentProviders;   ///< 组件类型注册表
    QVector<IExportProvider*>     m_vecExportProviders;      ///< 导出格式注册表
    QVector<IPanelProvider*>      m_vecPanelProviders;       ///< 面板注册表
    QVector<ITemplateProvider*>   m_vecTemplateProviders;    ///< 模板注册表
    QVector<IThemeProvider*>      m_vecThemeProviders;       ///< 主题注册表
    QVector<IUiStyleProvider*>    m_vecUiStyleProviders;     ///< UI 风格注册表
};

} // namespace bwm

#endif // BWM_PLUGIN_PLUGINHOST_H
