/**
 * @file PluginHost.cpp
 * @author zhangweimu
 * @brief 插件宿主实现。
 */
#include "plugin/PluginHost.h"

namespace bwm {

PluginHost::PluginHost(QObject* pParent)
    : QObject(pParent)
{
}

PluginHost::~PluginHost() = default;

// ---- 组件类型扩展点 ----

void PluginHost::registerComponentProvider(IComponentProvider* pProvider)
{
    if(pProvider) {
        m_vecComponentProviders.append(pProvider);
    }
}

const QVector<IComponentProvider*>& PluginHost::componentProviders() const
{
    return m_vecComponentProviders;
}

// ---- 导出格式扩展点 ----

void PluginHost::registerExportProvider(IExportProvider* pProvider)
{
    if(pProvider) {
        m_vecExportProviders.append(pProvider);
    }
}

const QVector<IExportProvider*>& PluginHost::exportProviders() const
{
    return m_vecExportProviders;
}

// ---- 面板扩展点 ----

void PluginHost::registerPanelProvider(IPanelProvider* pProvider)
{
    if(pProvider) {
        m_vecPanelProviders.append(pProvider);
    }
}

const QVector<IPanelProvider*>& PluginHost::panelProviders() const
{
    return m_vecPanelProviders;
}

// ---- 模板扩展点 ----

void PluginHost::registerTemplateProvider(ITemplateProvider* pProvider)
{
    if(pProvider) {
        m_vecTemplateProviders.append(pProvider);
    }
}

const QVector<ITemplateProvider*>& PluginHost::templateProviders() const
{
    return m_vecTemplateProviders;
}

// ---- 主题扩展点 ----

void PluginHost::registerThemeProvider(IThemeProvider* pProvider)
{
    if(pProvider) {
        m_vecThemeProviders.append(pProvider);
    }
}

const QVector<IThemeProvider*>& PluginHost::themeProviders() const
{
    return m_vecThemeProviders;
}

} // namespace bwm
