/**
 * @file BuiltinThemeProviders.cpp
 * @author zhangweimu
 * @brief 内置主题适配器实现。
 */
#include "plugin/builtin/BuiltinThemeProviders.h"

#include "theme/Theme.h"

namespace bwm {

QString BuiltinThemeProvider::providerId() const
{
    return QStringLiteral("builtin");
}

QVector<Theme> BuiltinThemeProvider::themes() const
{
    QVector<Theme> vecThemes;
    const QStringList names = ThemeManager::themeNames();
    for(const QString& strName : names) {
        vecThemes.append(ThemeManager::themeByName(strName));
    }
    return vecThemes;
}

} // namespace bwm
