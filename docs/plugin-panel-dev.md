# 面板/视图插件开发流程

> 对应接口：`IPanelProvider`（`src/plugin/IPanelProvider.h`）
>
> 当用户要求开发新的侧边面板（如属性检查器、历史面板、素材市场等）时，参照本文档并与用户讨论细节。

## 1. 概述

面板插件用于扩展主界面的侧边区域。实现 `IPanelProvider` 接口并注册到 `PluginHost` 后，新面板会自动以标签页形式挂载到主界面右侧面板区，与现有的"素材库"、"图层"标签页并列。

当前内置面板：素材库（`src/app/panels/AssetPanel`）、图层（`src/app/panels/LayerPanel`）。

> **注意**：当前内置面板是直接在 `MainWindow::createCentralWidget` 中创建的，尚未通过 `IPanelProvider` 注册。未来改造后会统一走注册流程。

## 2. 接口签名

```cpp
class IPanelProvider
{
public:
    virtual ~IPanelProvider() = default;

    // 面板唯一标识（如 "asset"、"layer"、"inspector"）
    virtual QString panelId() const = 0;

    // 用户可见的面板名称（如"素材库"、"属性"）
    virtual QString displayName() const = 0;

    // 创建面板的 QWidget 实例
    virtual QWidget* createPanel(QWidget* pParent,
                                 PluginHost* pHost,
                                 CanvasScene* pScene,
                                 CanvasView* pView,
                                 ProjectManager* pProjectManager) = 0;
};
```

### createPanel 参数说明

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `pParent` | `QWidget*` | 父 widget（标签页的父容器） |
| `pHost` | `PluginHost*` | 插件宿主（面板可通过它获取其他扩展点） |
| `pScene` | `CanvasScene*` | 画布场景（面板可直接操作场景，如图层面板读写组件列表） |
| `pView` | `CanvasView*` | 画布视图（面板可调用 centerOn 等方法） |
| `pProjectManager` | `ProjectManager*` | 项目管理器（面板可读写项目数据） |
| **返回值** | `QWidget*` | 面板根 widget（所有权归调用方，即 QTabWidget） |

## 3. 实现步骤

### 步骤一：确定面板功能与交互

> **讨论点**：与用户确认以下细节：
> - 面板功能（属性编辑 / 历史记录 / 素材市场 / 其他）
> - 需要访问哪些核心对象（场景 / 视图 / 项目管理器 / 插件宿主）
> - 与画布的交互方式（选中变化时面板如何响应 / 面板操作如何反馈到画布）
> - 是否需要发信号给 MainWindow（如数据变更后请求同步模型）

### 步骤二：创建面板 QWidget

先创建面板的 QWidget 子类（参考 `src/app/panels/LayerPanel` 的结构）：

```cpp
// src/app/panels/InspectorPanel.h
#ifndef BWM_APP_PANELS_INSPECTORPANEL_H
#define BWM_APP_PANELS_INSPECTORPANEL_H

#include <QWidget>

namespace bwm {

class CanvasScene;
class CanvasView;
class ComponentItem;

// 示例：属性检查器面板，显示并编辑选中组件的属性
class InspectorPanel : public QWidget
{
    Q_OBJECT
public:
    InspectorPanel(QWidget* pParent, CanvasScene* pScene, CanvasView* pView);

signals:
    // 属性变更后通知 MainWindow 同步模型
    void propertyChanged();

private slots:
    void onSelectionChanged();
    void onPropertyChanged();

private:
    CanvasScene* m_pScene;
    CanvasView* m_pView;
    // TODO: 添加属性编辑控件（QSpinBox / QColorDialog 等）
};

} // namespace bwm

#endif
```

### 步骤三：创建 Provider

```cpp
// src/plugin/builtin/InspectorPanelProvider.h
#ifndef BWM_PLUGIN_BUILTIN_INSPECTORPANELPROVIDER_H
#define BWM_PLUGIN_BUILTIN_INSPECTORPANELPROVIDER_H

#include "plugin/IPanelProvider.h"

namespace bwm {

class InspectorPanelProvider : public IPanelProvider
{
public:
    QString panelId() const override;
    QString displayName() const override;
    QWidget* createPanel(QWidget* pParent,
                         PluginHost* pHost,
                         CanvasScene* pScene,
                         CanvasView* pView,
                         ProjectManager* pProjectManager) override;
};

} // namespace bwm

#endif
```

```cpp
// src/plugin/builtin/InspectorPanelProvider.cpp
#include "plugin/builtin/InspectorPanelProvider.h"
#include "app/panels/InspectorPanel.h"

namespace bwm {

QString InspectorPanelProvider::panelId() const
{
    return QStringLiteral("inspector");
}

QString InspectorPanelProvider::displayName() const
{
    return QStringLiteral("属性");
}

QWidget* InspectorPanelProvider::createPanel(QWidget* pParent,
                                             PluginHost* pHost,
                                             CanvasScene* pScene,
                                             CanvasView* pView,
                                             ProjectManager* pProjectManager)
{
    (void)pHost;
    (void)pProjectManager;
    return new InspectorPanel(pParent, pScene, pView);
}

} // namespace bwm
```

### 步骤四：注册到 PluginHost

> **注意**：当前 `MainWindow::createCentralWidget` 直接创建各面板并添加到 `QTabWidget`。引入面板插件后，需要改为遍历 `PluginHost::panelProviders()` 自动创建。此改造暂未完成，当前需要在 `createCentralWidget` 中手动添加。

在 `MainWindow::createCentralWidget` 中添加：

```cpp
#include "plugin/builtin/InspectorPanelProvider.h"

// 创建 Provider 并注册
static InspectorPanelProvider* s_pInspector = new InspectorPanelProvider;
m_pHost->registerPanelProvider(s_pInspector);

// 创建面板并添加到标签页
auto* pInspectorPanel = s_pInspector->createPanel(m_pTabPanel, m_pHost, m_pScene, m_pView, m_pProjectManager);
m_pTabPanel->addTab(pInspectorPanel, s_pInspector->displayName());
```

### 步骤五：连接信号

在 `MainWindow` 中连接面板的信号到同步逻辑：

```cpp
// 假设 InspectorPanel 有 propertyChanged 信号
auto* pInspector = qobject_cast<InspectorPanel*>(pInspectorPanel);
if(pInspector) {
    connect(pInspector, &InspectorPanel::propertyChanged,
            this, &MainWindow::syncCanvasToModel);
}
```

### 步骤六：更新 CMakeLists.txt

在 `src/CMakeLists.txt` 的 `bwm` 目标源文件列表中添加面板源文件，在 `bwm_core` 中添加 Provider 源文件：

```cmake
# bwm 目标
app/panels/InspectorPanel.cpp

# bwm_core 目标
plugin/builtin/InspectorPanelProvider.cpp
```

## 4. 验证清单

- [ ] 编译通过
- [ ] 单元测试通过
- [ ] 主界面右侧标签页中出现新面板
- [ ] 面板能正确响应画布选中变化
- [ ] 面板上的操作能正确反馈到画布
- [ ] 面板操作后模型正确同步（标题出现脏标记 `*`）
- [ ] 切换页面后面板状态正确刷新

## 5. 注意事项

- 面板的 QWidget 所有权归 `QTabWidget`，不要手动 delete。
- 面板与画布的交互需注意信号回环问题（参考 `LayerPanel` 中 `m_bSyncing` 标志的用法）。
- 面板如果需要访问插件宿主查询其他扩展点（如列出所有组件类型），可通过 `createPanel` 传入的 `pHost` 指针。

## 6. 未来升级路径

当前面板注册需手动修改 `MainWindow::createCentralWidget`。未来改造为：`createCentralWidget` 遍历 `PluginHost::panelProviders()` 自动创建全部已注册面板，新增面板只需注册即可，无需修改 `MainWindow`。
