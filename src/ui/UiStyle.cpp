/**
 * @file UiStyle.cpp
 * @author zhangweimu
 * @brief 应用 UI 外观管理实现。
 */
#include "ui/UiStyle.h"

#include "settings/Settings.h"

#include <QWidget>

namespace bwm {

const QString UiStyleManager::kDarkId = QStringLiteral("dark");
const QString UiStyleManager::kLightId = QStringLiteral("light");
const QString UiStyleManager::kAutoId = QStringLiteral("auto");

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

const IUiStyleProvider* UiStyleManager::providerForStyle(const QString& rId)
{
    // 在所有已注册 Provider 中查找提供该风格 id 的那个
    for(const IUiStyleProvider* pProvider : registeredProviders()) {
        for(const UiStyleDescriptor& rDesc : pProvider->styles()) {
            if(rDesc.strId == rId) {
                return pProvider;
            }
        }
    }
    return nullptr;
}

bool UiStyleManager::applyStyleById(const QString& rId, QWidget* pMainWindow)
{
    const IUiStyleProvider* pProvider = providerForStyle(rId);
    return pProvider ? pProvider->applyStyle(rId, pMainWindow) : false;
}

bool UiStyleManager::paintWindowBackground(QWidget* pMainWindow, QPainter& rPainter)
{
    const IUiStyleProvider* pProvider = providerForStyle(currentStyleId());
    if(pProvider) {
        return pProvider->paintBackground(currentStyleId(), pMainWindow, rPainter);
    }
    return false;
}

QColor UiStyleManager::canvasBackgroundColor()
{
    const IUiStyleProvider* pProvider = providerForStyle(currentStyleId());
    if(pProvider) {
        return pProvider->canvasBackgroundColor(currentStyleId());
    }
    return QColor();
}

bool UiStyleManager::applyCurrentStyle(QWidget* pMainWindow)
{
    if(applyStyleById(currentStyleId(), pMainWindow)) {
        return true;
    }
    // 当前风格不可用（如 Provider 卸载、持久化值失效），回退到第一个可用风格。
    // 注意：回退只在本次运行生效，不修改持久化设置。
    const QVector<UiStyleDescriptor> vecStyles = availableStyles();
    if(!vecStyles.isEmpty()) {
        return applyStyleById(vecStyles.first().strId, pMainWindow);
    }
    return false;
}

} // namespace bwm
