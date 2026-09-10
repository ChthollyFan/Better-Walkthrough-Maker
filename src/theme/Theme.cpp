/**
 * @file Theme.cpp
 * @author zhangweimu
 * @brief 主题包实现：内置画布配色主题表。
 */
#include "theme/Theme.h"

#include "settings/Settings.h"

namespace bwm {

namespace {

// 内置主题表：目前仅保留「浅色页面」一种画布配色。
// 界面深浅色由 UI 外观（ui/UiStyle.h）负责，与画布配色相互独立。
const QVector<Theme> kBuiltinThemes = {
    {
        QStringLiteral("浅色页面"),
        QColor(29, 53, 87),         // 主色：深蓝
        QColor(69, 123, 157),       // 辅色：蓝
        QColor(255, 255, 255),      // 背景：白
        QColor(33, 33, 33),         // 文字：近黑
    },
};

} // namespace

QStringList ThemeManager::themeNames()
{
    QStringList names;
    for (const Theme& rTheme : kBuiltinThemes) {
        names.append(rTheme.strName);
    }
    return names;
}

Theme ThemeManager::themeByName(const QString& rName)
{
    for (const Theme& rTheme : kBuiltinThemes) {
        if (rTheme.strName == rName) {
            return rTheme;
        }
    }
    return kBuiltinThemes.first();
}

QString ThemeManager::currentThemeName()
{
    return Settings::themeName();
}

void ThemeManager::setCurrentThemeName(const QString& rName)
{
    Settings::setThemeName(rName);
}

Theme ThemeManager::currentTheme()
{
    return themeByName(currentThemeName());
}

} // namespace bwm
