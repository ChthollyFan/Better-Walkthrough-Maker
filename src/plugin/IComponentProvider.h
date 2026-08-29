/**
 * @file IComponentProvider.h
 * @author zhangweimu
 * @brief 组件类型插件接口：第三方可实现此接口注册新的画布组件类型。
 *
 * 实现此接口并注册到 PluginHost 后，对应的组件类型会自动出现在
 * "插入"菜单和工具栏中，用户选择后由 createComponent 生成默认数据
 * 并添加到画布。
 *
 * 当前为编译时静态注册（内置适配器在 BuiltinComponentProviders 中实现）。
 * 未来插件以动态库形式加载时，只需实现此接口并在初始化时调用
 * PluginHost::registerComponentProvider 即可。
 */
#ifndef BWM_PLUGIN_ICOMPONENTPROVIDER_H
#define BWM_PLUGIN_ICOMPONENTPROVIDER_H

#include <QString>

#include "core/Component.h"
#include "plugin/PluginContext.h"

class QWidget;   // 前向声明：showInputDialog 参数用

namespace bwm {

/**
 * @brief 组件类型插件接口。
 *
 * 每个实现代表一种可插入画布的组件类型（如图片、文本、矩形、贴纸等）。
 * 接口设计为纯虚函数，实现方负责创建默认 Component 数据。
 */
class IComponentProvider
{
public:
    virtual ~IComponentProvider() = default;

    /**
     * @brief 组件类型唯一标识（字符串形式，如 "image"、"text"、"shape.rect"）。
     *
     * 用字符串而非枚举值，便于第三方插件扩展而不修改核心枚举。
     * 内置类型与 E_COMPONENT_TYPE 的映射见 BuiltinComponentProviders。
     */
    virtual QString typeId() const = 0;

    /**
     * @brief 用户可见的显示名称（如"图片"、"文本"、"矩形"）。
     * 用于菜单项与工具栏按钮的文字。
     */
    virtual QString displayName() const = 0;

    /**
     * @brief 菜单路径（如 "插入/形状/矩形" 或 "插入/图片"）。
     *
     * MenuBuilder 按此路径自动构建多级子菜单。
     * 路径以 '/' 分隔，最后一段为叶子菜单项名称。
     */
    virtual QString menuPath() const = 0;

    /**
     * @brief 创建该类型组件的默认数据。
     *
     * @param rContext  插件上下文（提供主题配色、画布尺寸等）
     * @return          填充了默认值的 Component（strId/nZOrder 由场景后续分配）
     *
     * 实现方应在此设置 eType、size、以及类型特有数据（如默认颜色、文本等）。
     */
    virtual Component createComponent(const PluginContext& rContext) const = 0;

    /**
     * @brief 是否需要在插入前弹出输入对话框（如文本组件需输入文字内容）。
     *
     * 默认返回 false（直接插入）。返回 true 时，框架会调用 showInputDialog。
     */
    virtual bool requiresInputDialog() const { return false; }

    /**
     * @brief 弹出输入对话框，让用户在插入前设置初始内容。
     *
     * @param pParent     父窗口（对话框的 parent）
     * @param rComponent  待插入的组件数据（已由 createComponent 填充默认值，
     *                    此方法可在此基础上修改）
     * @param rContext    插件上下文
     * @return            true 表示用户确认（继续插入），false 表示取消
     *
     * 默认实现返回 false（不应被调用，requiresInputDialog 返回 false 时框架不调此方法）。
     */
    virtual bool showInputDialog(QWidget* pParent, Component& rComponent,
                                 const PluginContext& rContext) const
    {
        (void)pParent;
        (void)rComponent;
        (void)rContext;
        return false;
    }
};

} // namespace bwm

#endif // BWM_PLUGIN_ICOMPONENTPROVIDER_H
