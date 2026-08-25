/**
 * @file Theme.cpp
 * @author zhangweimu
 * @brief 主题包实现：内置两套配色（深色游戏风 / 浅色简洁风）。
 */
#include "theme/Theme.h"

#include "settings/Settings.h"

namespace bwm {

namespace {

// 内置主题表
const QVector<Theme> kBuiltinThemes = {
    {
        QStringLiteral("深色游戏风"),
        QColor(230, 57, 70),        // 主色：高饱和红
        QColor(69, 123, 157),       // 辅色：蓝
        QColor(30, 30, 46),         // 背景：深蓝灰
        QColor(237, 237, 237),      // 文字：近白
    },
    {
        QStringLiteral("浅色简洁风"),
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
