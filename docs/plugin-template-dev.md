# 模板包插件开发流程

> 对应接口：`ITemplateProvider`（`src/plugin/ITemplateProvider.h`）
>
> 当用户要求开发模板包插件（提供攻略模板集合）时，参照本文档并与用户讨论细节。

## 1. 概述

模板包插件用于提供攻略模板集合。实现 `ITemplateProvider` 接口并注册到 `PluginHost` 后，提供的模板会出现在"新建攻略"的模板选择列表中，与内置模板合并显示。用户选择模板后，框架复制模板的全部页面并重新生成组件 id。

当前内置实现：`BuiltinTemplateProvider`（`src/plugin/builtin/BuiltinTemplateProviders.cpp`），合并了 `BuiltinTemplates`（内置模板）与 `TemplateManager`（项目用户模板）。

## 2. 接口签名

```cpp
class ITemplateProvider
{
public:
    virtual ~ITemplateProvider() = default;

    // 模板包唯一标识（如 "builtin"、"community-pack"）
    virtual QString providerId() const = 0;

    // 提供全部模板列表
    virtual QVector<Template> templates(const QString& rProjectDir) const = 0;
};
```

### Template 结构体

```cpp
struct Template
{
    QString strName;                        // 模板名（列表中显示）
    E_WALKTHROUGH_TYPE eType;               // 攻略类型
    QString strDescription;                 // 说明（列表 tooltip）
    QVector<Page> vecPages;                 // 页面布局
};
```

### templates 参数说明

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `rProjectDir` | `const QString&` | 当前项目目录（某些模板包可能依赖项目路径，如读取项目内素材） |
| **返回值** | `QVector<Template>` | 模板列表 |

## 3. 实现步骤

### 步骤一：确定模板包内容

> **讨论点**：与用户确认以下细节：
> - 模板包名称与标识
> - 包含哪些模板（每套模板的名称、攻略类型、页面数量）
> - 每套模板的页面布局（组件的位置、尺寸、内容）
> - 模板数据来源（硬编码 / JSON 文件 / 网络下载）
> - 是否依赖项目目录中的资源

### 步骤二：创建 Provider 头文件

```cpp
// src/plugin/builtin/SeasonalTemplateProvider.h
#ifndef BWM_PLUGIN_BUILTIN_SEASONALTEMPLATEPROVIDER_H
#define BWM_PLUGIN_BUILTIN_SEASONALTEMPLATEPROVIDER_H

#include "plugin/ITemplateProvider.h"

namespace bwm {

// 示例：节日主题模板包
class SeasonalTemplateProvider : public ITemplateProvider
{
public:
    QString providerId() const override;
    QVector<Template> templates(const QString& rProjectDir) const override;
};

} // namespace bwm

#endif
```

### 步骤三：实现 Provider

```cpp
// src/plugin/builtin/SeasonalTemplateProvider.cpp
#include "plugin/builtin/SeasonalTemplateProvider.h"

#include "core/Project.h"

namespace bwm {

QString SeasonalTemplateProvider::providerId() const
{
    return QStringLiteral("seasonal");
}

QVector<Template> SeasonalTemplateProvider::templates(const QString& rProjectDir) const
{
    (void)rProjectDir;
    QVector<Template> vecTemplates;

    // 示例：春节主题封面模板
    Template springFestival;
    springFestival.strName = QStringLiteral("春节封面");
    springFestival.eType = E_WALKTHROUGH_TYPE_COVER;
    springFestival.strDescription = QStringLiteral("春节主题封面模板，红色喜庆风格");
    Page page;
    page.strName = QStringLiteral("封面");
    page.size = QSize(1080, 1440);
    // TODO: 添加组件到页面（标题文字、装饰贴纸等）
    springFestival.vecPages.append(page);
    vecTemplates.append(springFestival);

    return vecTemplates;
}

} // namespace bwm
```

### 步骤四：注册到 PluginHost

在 `src/plugin/builtin/BuiltinPluginRegistrar.cpp` 中添加：

```cpp
#include "plugin/builtin/SeasonalTemplateProvider.h"

// 在 registerBuiltinPlugins 函数体内添加：
static SeasonalTemplateProvider* s_pSeasonal = new SeasonalTemplateProvider;
pHost->registerTemplateProvider(s_pSeasonal);
```

### 步骤五：更新 CMakeLists.txt

在 `src/CMakeLists.txt` 的 `bwm_core` 目标源文件列表中添加：

```cmake
plugin/builtin/SeasonalTemplateProvider.cpp
```

## 4. 验证清单

- [ ] 编译通过
- [ ] 单元测试通过
- [ ] "新建攻略"对话框模板列表中出现新模板
- [ ] 模板列表显示名称、页数、tooltip 说明
- [ ] 选择模板后创建的攻略包含正确的页面和组件
- [ ] 创建后组件 id 已重新生成（无重复）

## 5. 注意事项

- 模板中的组件 id 会在创建攻略时由框架自动重新生成（`QUuid::createUuid`），Provider 无需关心唯一性。
- 模板列表会在 `ProjectTreePanel::onAddWalkthrough` 中被遍历合并，多个 Provider 的模板会一起显示。
- 如果模板需要引用外部资源文件（如图片），建议在 `templates()` 中用 `rProjectDir` 解析路径。

## 6. 未来升级路径

当前为编译时静态注册。未来模板包可打包为独立目录（含 `templates.json` + 资源文件），由动态库插件加载后通过 `ITemplateProvider` 提供。
