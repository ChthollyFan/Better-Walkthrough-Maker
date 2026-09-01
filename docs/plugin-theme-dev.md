# 主题插件开发流程

> 对应接口：`IThemeProvider`（`src/plugin/IThemeProvider.h`）
>
> 当用户要求开发主题插件（提供新的配色方案）时，参照本文档并与用户讨论细节。

## 1. 概述

主题插件用于提供新的配色方案。实现 `IThemeProvider` 接口并注册到 `PluginHost` 后，提供的主题会出现在"主题"菜单中，与内置主题合并显示。用户选择主题后，框架调用 `ThemeManager::setCurrentThemeName` 切换并刷新画布。

当前内置实现：`BuiltinThemeProvider`（`src/plugin/builtin/BuiltinThemeProviders.cpp`），通过 `ThemeManager` 获取内置主题。

## 2. 接口签名

```cpp
class IThemeProvider
{
public:
    virtual ~IThemeProvider() = default;

    // 主题包唯一标识（如 "builtin"、"dark-pack"）
    virtual QString providerId() const = 0;

    // 提供全部主题配色列表
    virtual QVector<Theme> themes() const = 0;
};
```

### Theme 结构体

```cpp
struct Theme
{
    QString strName;              // 主题名（菜单中显示，也是切换主题的键）
    QColor primaryColor;          // 主色（贴纸、边框等强调元素）
    QColor secondaryColor;        // 辅色（形状填充等）
    QColor backgroundColor;       // 页面背景色
    QColor textColor;             // 默认文字色
};
```

## 3. 实现步骤

### 步骤一：确定主题配色

> **讨论点**：与用户确认以下细节：
> - 主题包名称与标识
> - 包含哪些主题（每套主题的名称）
> - 每套主题的 4 个颜色值（主色 / 辅色 / 背景色 / 文字色）
> - 主题名称是否有冲突（同名主题会重复出现在菜单中）

### 步骤二：创建 Provider 头文件

```cpp
// src/plugin/builtin/DarkThemePackProvider.h
#ifndef BWM_PLUGIN_BUILTIN_DARKTHEMEPACKPROVIDER_H
#define BWM_PLUGIN_BUILTIN_DARKTHEMEPACKPROVIDER_H

#include "plugin/IThemeProvider.h"

namespace bwm {

// 示例：暗色主题包
class DarkThemePackProvider : public IThemeProvider
{
public:
    QString providerId() const override;
    QVector<Theme> themes() const override;
};

} // namespace bwm

#endif
```

### 步骤三：实现 Provider

```cpp
// src/plugin/builtin/DarkThemePackProvider.cpp
#include "plugin/builtin/DarkThemePackProvider.h"

namespace bwm {

QString DarkThemePackProvider::providerId() const
{
    return QStringLiteral("dark-pack");
}

QVector<Theme> DarkThemePackProvider::themes() const
{
    QVector<Theme> vecThemes;

    // 深空灰主题
    Theme darkGray;
    darkGray.strName = QStringLiteral("深空灰");
    darkGray.primaryColor = QColor(100, 150, 200);
    darkGray.secondaryColor = QColor(60, 60, 70);
    darkGray.backgroundColor = QColor(30, 30, 35);
    darkGray.textColor = QColor(220, 220, 225);
    vecThemes.append(darkGray);

    // 墨绿主题
    Theme forestGreen;
    forestGreen.strName = QStringLiteral("墨绿");
    forestGreen.primaryColor = QColor(80, 160, 100);
    forestGreen.secondaryColor = QColor(50, 80, 60);
    forestGreen.backgroundColor = QColor(25, 40, 30);
    forestGreen.textColor = QColor(210, 230, 215);
    vecThemes.append(forestGreen);

    return vecThemes;
}

} // namespace bwm
```

### 步骤四：注册到 PluginHost

在 `src/plugin/builtin/BuiltinPluginRegistrar.cpp` 中添加：

```cpp
#include "plugin/builtin/DarkThemePackProvider.h"

// 在 registerBuiltinPlugins 函数体内添加：
static DarkThemePackProvider* s_pDarkPack = new DarkThemePackProvider;
pHost->registerThemeProvider(s_pDarkPack);
```

### 步骤五：更新 CMakeLists.txt

在 `src/CMakeLists.txt` 的 `bwm_core` 目标源文件列表中添加：

```cmake
plugin/builtin/DarkThemePackProvider.cpp
```

## 4. 验证清单

- [ ] 编译通过
- [ ] 单元测试通过
- [ ] "主题"菜单中出现新主题
- [ ] 选择新主题后画布背景色正确变化
- [ ] 选择新主题后新插入的组件使用新配色（主色 / 辅色 / 文字色）
- [ ] 主题选择持久化（关闭重启后保持上次选择）
- [ ] 已存在的组件颜色不受影响（主题仅影响新插入组件的默认色）

## 5. 注意事项

- 主题切换通过 `ThemeManager::setCurrentThemeName` 实现，它依赖主题名匹配。如果插件提供的主题名与内置主题重复，`ThemeManager::themeByName` 会返回先注册的那个。
- 主题仅影响**新插入组件的默认颜色**和**画布背景色**，不会修改已有组件的颜色。
- `ThemeManager` 当前是静态类，内置主题硬编码在 `src/theme/Theme.cpp` 中。插件提供的主题通过 `PluginHost` 合并到菜单，但 `ThemeManager::currentTheme()` 仍返回内置主题。如果插件主题需要被 `ThemeManager` 管理，需要扩展 `ThemeManager` 支持动态注册。

## 6. 未来升级路径

当前为编译时静态注册。未来主题包可打包为 JSON 文件（含 4 个颜色值），由动态库插件或 `ThemeManager` 直接读取。`ThemeManager` 需扩展为支持动态主题注册（`registerTheme(const Theme&)`）。
