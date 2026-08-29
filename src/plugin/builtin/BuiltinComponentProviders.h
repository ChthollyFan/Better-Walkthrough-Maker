/**
 * @file BuiltinComponentProviders.h
 * @author zhangweimu
 * @brief 内置组件类型适配器：把现有的图片/文本/形状/表格/贴纸组件
 *        适配为 IComponentProvider 插件接口。
 *
 * 每个 Provider 类对应一种内置组件类型。这些类在应用初始化时
 * 由 MainWindow 创建并注册到 PluginHost。
 *
 * 适配器模式说明：这些类本身不包含业务逻辑，仅把 MainWindow 中
 * 原来硬编码的 onAddXxxComponent 逻辑迁移到接口方法中，
 * 使组件类型的扩展点通过统一接口暴露。
 */
#ifndef BWM_PLUGIN_BUILTIN_BUILTINCOMPONENTPROVIDERS_H
#define BWM_PLUGIN_BUILTIN_BUILTINCOMPONENTPROVIDERS_H

#include "plugin/IComponentProvider.h"

namespace bwm {

/**
 * @brief 图片组件 Provider。
 *
 * 插入时弹出文件选择对话框，用户选择图片后复制到项目 assets/ 目录。
 */
class ImageComponentProvider : public IComponentProvider
{
public:
    QString typeId() const override;
    QString displayName() const override;
    QString menuPath() const override;
    Component createComponent(const PluginContext& rContext) const override;
    bool requiresInputDialog() const override;
    bool showInputDialog(QWidget* pParent, Component& rComponent,
                         const PluginContext& rContext) const override;
};

/**
 * @brief 文本组件 Provider。
 * 插入时弹出文本输入对话框。
 */
class TextComponentProvider : public IComponentProvider
{
public:
    QString typeId() const override;
    QString displayName() const override;
    QString menuPath() const override;
    Component createComponent(const PluginContext& rContext) const override;
    bool requiresInputDialog() const override;
    bool showInputDialog(QWidget* pParent, Component& rComponent,
                         const PluginContext& rContext) const override;
};

/**
 * @brief 形状组件 Provider（参数化：矩形/圆角矩形/椭圆/线条）。
 * menuPath 按 shapeType 返回不同的路径。
 */
class ShapeComponentProvider : public IComponentProvider
{
public:
    /**
     * @param eShapeType  形状子类型
     * @param nShapeType  形状子类型的 int 值（用于 typeId 区分）
     */
    ShapeComponentProvider(E_SHAPE_TYPE eShapeType, int nShapeType);

    QString typeId() const override;
    QString displayName() const override;
    QString menuPath() const override;
    Component createComponent(const PluginContext& rContext) const override;

private:
    E_SHAPE_TYPE m_eShapeType;   ///< 形状子类型
    int m_nShapeType;            ///< 形状子类型 int 值（typeId 用）
};

/**
 * @brief 表格组件 Provider。
 * 插入时使用默认装备表模板数据。
 */
class TableComponentProvider : public IComponentProvider
{
public:
    QString typeId() const override;
    QString displayName() const override;
    QString menuPath() const override;
    Component createComponent(const PluginContext& rContext) const override;
};

/**
 * @brief 贴纸组件 Provider（参数化：标题线/角标/星标/箭头/分割线/卡片边框）。
 */
class StickerComponentProvider : public IComponentProvider
{
public:
    /**
     * @param eStickerType  贴纸子类型
     * @param nStickerType  贴纸子类型的 int 值（用于 typeId 区分）
     */
    StickerComponentProvider(E_STICKER_TYPE eStickerType, int nStickerType);

    QString typeId() const override;
    QString displayName() const override;
    QString menuPath() const override;
    Component createComponent(const PluginContext& rContext) const override;

private:
    E_STICKER_TYPE m_eStickerType;   ///< 贴纸子类型
    int m_nStickerType;              ///< 贴纸子类型 int 值
};

} // namespace bwm

#endif // BWM_PLUGIN_BUILTIN_BUILTINCOMPONENTPROVIDERS_H
