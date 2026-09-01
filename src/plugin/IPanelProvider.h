/**
 * @file IPanelProvider.h
 * @author zhangweimu
 * @brief 面板/视图插件接口：第三方可实现此接口注册新的侧边面板。
 *
 * 实现此接口并注册到 PluginHost 后，对应面板会自动以标签页形式
 * 挂载到主界面右侧面板区。当前内置面板：素材库、图层。
 *
 * 未来可扩展属性检查器、历史面板、素材市场等。
 */
#ifndef BWM_PLUGIN_IPANELPROVIDER_H
#define BWM_PLUGIN_IPANELPROVIDER_H

#include <QString>
#include <QWidget>

namespace bwm {

class CanvasScene;
class CanvasView;
class ProjectManager;
class PluginHost;

/**
 * @brief 面板插件接口。
 *
 * 每个实现代表一个可挂载到主界面的侧边面板。
 * 面板通过 PluginHost 获取其他扩展点能力（如查询组件类型列表）。
 */
class IPanelProvider
{
public:
    virtual ~IPanelProvider() = default;

    /**
     * @brief 面板唯一标识（如 "asset"、"layer"）。
     */
    virtual QString panelId() const = 0;

    /**
     * @brief 用户可见的面板名称（如"素材库"、"图层"）。
     * 用于标签页标题。
     */
    virtual QString displayName() const = 0;

    /**
     * @brief 创建面板的 QWidget 实例。
     *
     * @param pParent        父 widget
     * @param pHost          插件宿主（面板可通过它获取其他扩展点）
     * @param pScene         画布场景（面板可直接操作场景，如图层面板）
     * @param pView          画布视图
     * @param pProjectManager 项目管理器
     * @return               面板根 widget（所有权归调用方）
     *
     * 框架在主窗口初始化时调用此方法，将返回的 widget 添加到标签页。
     */
    virtual QWidget* createPanel(QWidget* pParent,
                                 PluginHost* pHost,
                                 CanvasScene* pScene,
                                 CanvasView* pView,
                                 ProjectManager* pProjectManager) = 0;
};

} // namespace bwm

#endif // BWM_PLUGIN_IPANELPROVIDER_H
