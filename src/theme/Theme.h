/**
 * @file Theme.h
 * @author zhangweimu
 * @brief 主题包：配色方案与应用（M5）。
 */
#ifndef BWM_THEME_THEME_H
#define BWM_THEME_THEME_H

#include <QColor>
#include <QString>
#include <QStringList>

namespace bwm {

// 主题配色：主色 / 辅色 / 页面背景 / 默认文字色。
struct Theme {
    QString strName;              // 主题名
    QColor primaryColor;          // 主色（贴纸、边框等强调元素）
    QColor secondaryColor;        // 辅色（形状填充等）
    QColor backgroundColor;       // 页面背景色
    QColor textColor;             // 默认文字色
};

// 主题管理：内置主题与当前主题（持久化于全局设置）。
class ThemeManager {
public:
    // 全部内置主题名
    static QStringList themeNames();
    // 按名称取主题（未知名称回退第一个内置主题）
    static Theme themeByName(const QString& rName);
    // 当前主题
    static QString currentThemeName();
    static void setCurrentThemeName(const QString& rName);
    static Theme currentTheme();
};

} // namespace bwm

#endif // BWM_THEME_THEME_H
