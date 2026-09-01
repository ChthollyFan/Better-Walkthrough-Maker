# 组件类型插件开发流程

> 对应接口：`IComponentProvider`（`src/plugin/IComponentProvider.h`）
>
> 当用户要求开发新的画布组件类型（如图表、二维码、进度条等）时，参照本文档并与用户讨论细节。

## 1. 概述

组件类型插件用于扩展画布上可插入的元素种类。实现 `IComponentProvider` 接口并注册到 `PluginHost` 后，新组件类型会自动出现在"插入"菜单和工具栏中，用户选择后由框架调用 `createComponent` 生成默认数据并添加到画布。

## 2. 接口签名

```cpp
class IComponentProvider
{
public:
    virtual ~IComponentProvider() = default;

    // 组件类型唯一标识（如 "image"、"text"、"shape.rect"）
    virtual QString typeId() const = 0;

    // 用户可见的显示名称（如"图片"、"矩形"）
    virtual QString displayName() const = 0;

    // 菜单路径（如 "插入/形状/矩形"，以 '/' 分隔）
    virtual QString menuPath() const = 0;

    // 创建默认 Component 数据（strId/nZOrder 由场景后续分配）
    virtual Component createComponent(const PluginContext& rContext) const = 0;

    // 是否需要在插入前弹出输入对话框（默认 false）
    virtual bool requiresInputDialog() const { return false; }

    // 弹出输入对话框（requiresInputDialog 返回 true 时由框架调用）
    virtual bool showInputDialog(QWidget* pParent, Component& rComponent,
                                 const PluginContext& rContext) const { return false; }
};
```

### PluginContext 提供的环境信息

```cpp
struct PluginContext
{
    Theme theme;                    // 当前主题配色（主色/辅色/背景色/文字色）
    QSize defaultPageSize;          // 全局默认画布尺寸
    QString projectDirectory;       // 当前项目目录（无项目时为空）
    Page* pCurrentPage = nullptr;   // 当前编辑的页面指针（无选中页面时为 nullptr）
};
```

## 3. 实现步骤

### 步骤一：确定组件数据模型

如果新组件类型需要专有数据字段，需要在 `src/core/Component.h` 的 `Component` 结构体中添加对应的 data 结构体和成员。如果新组件可以复用现有 `Component` 字段（如仅用 `shapeData` 配合不同的 `eType`），则跳过此步。

> **讨论点**：与用户确认新组件是否需要扩展 `Component` 数据结构，还是复用现有字段。

### 步骤二：创建 Provider 头文件

在 `src/plugin/builtin/` 下创建头文件（内置插件）或 `src/plugin/custom/`（第三方插件）：

```cpp
// src/plugin/builtin/MyComponentProvider.h
#ifndef BWM_PLUGIN_BUILTIN_MYCOMPONENTPROVIDER_H
#define BWM_PLUGIN_BUILTIN_MYCOMPONENTPROVIDER_H

#include "plugin/IComponentProvider.h"

namespace bwm {

// 示例：二维码组件 Provider
class QrCodeComponentProvider : public IComponentProvider
{
public:
    QString typeId() const override;
    QString displayName() const override;
    QString menuPath() const override;
    Component createComponent(const PluginContext& rContext) const override;
};

} // namespace bwm

#endif
```

### 步骤三：实现 Provider

```cpp
// src/plugin/builtin/MyComponentProvider.cpp
#include "plugin/builtin/MyComponentProvider.h"

namespace bwm {

QString QrCodeComponentProvider::typeId() const
{
    return QStringLiteral("builtin.qrcode");
}

QString QrCodeComponentProvider::displayName() const
{
    return QStringLiteral("二维码");
}

QString QrCodeComponentProvider::menuPath() const
{
    return QStringLiteral("插入/二维码");
}

Component QrCodeComponentProvider::createComponent(const PluginContext& rContext) const
{
    (void)rContext;
    Component component;
    component.eType = E_COMPONENT_TYPE_SHAPE;  // 复用形状类型或自定义新类型
    component.size = QSizeF(200, 200);
    // TODO: 设置组件特有默认数据
    return component;
}

} // namespace bwm
```

### 步骤四：注册到 PluginHost

在 `src/plugin/builtin/BuiltinPluginRegistrar.cpp` 的 `registerBuiltinPlugins` 函数中添加注册：

```cpp
#include "plugin/builtin/MyComponentProvider.h"

// 在 registerBuiltinPlugins 函数体内添加：
static QrCodeComponentProvider* s_pQrCode = new QrCodeComponentProvider;
pHost->registerComponentProvider(s_pQrCode);
```

### 步骤五：更新 CMakeLists.txt

在 `src/CMakeLists.txt` 的 `bwm_core` 目标源文件列表中添加：

```cmake
plugin/builtin/MyComponentProvider.cpp
```

### 步骤六：实现渲染（如需要）

如果新组件有自定义渲染逻辑，需要在 `src/core/ComponentPainter.cpp` 中添加对应的绘制分支。

> **讨论点**：与用户确认渲染方式——是用 QPainter 程序绘制，还是加载图片资源。

## 4. 验证清单

- [ ] 编译通过（`cmake --build build`）
- [ ] 单元测试通过（`ctest --test-dir build --output-on-failure`）
- [ ] "插入"菜单中出现新组件类型
- [ ] 工具栏中出现新组件按钮（若 `menuPath` 只有一级）
- [ ] 点击后画布上出现默认组件
- [ ] 组件可正常移动、缩放、删除
- [ ] 组件可正常保存与加载（序列化正确）

## 5. 未来升级路径

当前为编译时静态注册。未来实现动态库加载后：

1. 将 Provider 实现编译为独立 `.dll`/`.so`
2. 在动态库的初始化函数中调用 `PluginHost::registerComponentProvider`
3. `PluginHost::loadPlugins()`（TODO）扫描 `plugins/` 目录并通过 `QPluginLoader` 加载
4. 无需修改主程序代码即可新增组件类型
