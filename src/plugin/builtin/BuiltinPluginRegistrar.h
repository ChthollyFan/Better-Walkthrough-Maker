/**
 * @file BuiltinPluginRegistrar.h
 * @author zhangweimu
 * @brief 内置插件注册器：把所有内置适配器注册到 PluginHost。
 *
 * 在应用初始化时调用 registerBuiltinPlugins(pHost)，
 * 将内置的组件类型、导出格式、模板、主题 Provider 注册到 PluginHost。
 * 面板 Provider 不在此注册（面板由 MainWindow 直接创建并挂载）。
 */
#ifndef BWM_PLUGIN_BUILTIN_BUILTINPLUGINREGISTRAR_H
#define BWM_PLUGIN_BUILTIN_BUILTINPLUGINREGISTRAR_H

namespace bwm {

class PluginHost;

/**
 * @brief 注册全部内置 Provider 到 PluginHost。
 * @param pHost  插件宿主
 *
 * 内部创建各 Provider 实例并注册。Provider 实例为静态对象，
 * 生命周期贯穿整个应用，不随 PluginHost 销毁（指针安全）。
 */
void registerBuiltinPlugins(PluginHost* pHost);

} // namespace bwm

#endif // BWM_PLUGIN_BUILTIN_BUILTINPLUGINREGISTRAR_H
