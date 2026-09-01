/**
 * @file UiStyle.cpp
 * @author zhangweimu
 * @brief 应用 UI 外观管理实现。
 */
#include "ui/UiStyle.h"

#include "settings/Settings.h"

#include <QWidget>

namespace bwm {

const QString UiStyleManager::kSystemId = QStringLiteral("system");
const QString UiStyleManager::kAcrylicDarkId = QStringLiteral("acrylic-dark");
const QString UiStyleManager::kAcrylicLightId = QStringLiteral("acrylic-light");

namespace {

// 已注册 Provider 列表（静态存活，指针安全）
QVector<IUiStyleProvider*>& registeredProviders()
{
    static QVector<IUiStyleProvider*> s_vecProviders;
    return s_vecProviders;
}

} // namespace

QString UiStyleManager::currentStyleId()
{
    return Settings::uiStyle();
}

void UiStyleManager::setCurrentStyleId(const QString& rId)
{
    Settings::setUiStyle(rId);
}

QVector<UiStyleDescriptor> UiStyleManager::availableStyles()
{
    QVector<UiStyleDescriptor> vecStyles;
    // 仅合并各 Provider 提供的风格。system 不暴露给用户，仅作运行时 fallback。
    for(const IUiStyleProvider* pProvider : registeredProviders()) {
        vecStyles.append(pProvider->styles());
    }
    return vecStyles;
}

void UiStyleManager::ensureCurrentStyleAvailable()
{
    // 若持久化的当前风格不在可用列表内（如用户之前选过已删除的 system，
    // 或 Provider 卸载导致风格失效），回退到第一个可用风格并持久化。
    // 在菜单构建前调用，避免菜单出现"无选中项"。
    const QString strId = currentStyleId();
    const QVector<UiStyleDescriptor> vecStyles = availableStyles();
    for(const UiStyleDescriptor& rDesc : vecStyles) {
        if(rDesc.strId == strId) {
            return;   // 当前风格可用
        }
    }
    if(!vecStyles.isEmpty()) {
        setCurrentStyleId(vecStyles.first().strId);
    }
}

void UiStyleManager::registerProvider(IUiStyleProvider* pProvider)
{
    if(pProvider && !registeredProviders().contains(pProvider)) {
        registeredProviders().append(pProvider);
    }
}

bool UiStyleManager::applyStyleById(const QString& rId, QWidget* pMainWindow)
{
    if(rId == kSystemId) {
        // system 风格：不做任何原生效果，返回 true 表示"已处理"
        return true;
    }
    for(const IUiStyleProvider* pProvider : registeredProviders()) {
        // 检查该 Provider 是否提供此风格
        bool bProvides = false;
        for(const UiStyleDescriptor& desc : pProvider->styles()) {
            if(desc.strId == rId) {
                bProvides = true;
                break;
            }
        }
        if(bProvides && pProvider->applyStyle(rId, pMainWindow)) {
            return true;
        }
    }
    return false;
}

bool UiStyleManager::applyCurrentStyle(QWidget* pMainWindow)
{
    const QString strId = currentStyleId();
    if(applyStyleById(strId, pMainWindow)) {
        return true;
    }
    // 当前风格应用失败（如非 Windows 平台选了亚克力），回退到 system
    // 注意：回退时不修改持久化设置，仅在本次运行降级
    applyStyleById(kSystemId, pMainWindow);
    return false;
}

} // namespace bwm
