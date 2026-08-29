/**
 * @file BuiltinPluginRegistrar.cpp
 * @author zhangweimu
 * @brief 内置插件注册器实现。
 *
 * 在函数局部静态变量中创建各 Provider 实例，确保：
 * 1. 线程安全的延迟初始化（C++11 保证）。
 * 2. 生命周期贯穿整个应用，PluginHost 仅持有指针。
 * 3. 多次调用 registerBuiltinPlugins 不会重复创建。
 */
#include "plugin/builtin/BuiltinPluginRegistrar.h"

#include "plugin/PluginHost.h"
#include "plugin/builtin/BuiltinComponentProviders.h"
#include "plugin/builtin/BuiltinExportProviders.h"
#include "plugin/builtin/BuiltinTemplateProviders.h"
#include "plugin/builtin/BuiltinThemeProviders.h"

namespace bwm {

void registerBuiltinPlugins(PluginHost* pHost)
{
    if(!pHost) {
        return;
    }

    // ---- 组件类型 Provider ----
    // 使用静态变量确保只创建一次，Provider 指针安全
    static ImageComponentProvider* s_pImage = new ImageComponentProvider;
    static TextComponentProvider* s_pText = new TextComponentProvider;
    static TableComponentProvider* s_pTable = new TableComponentProvider;

    // 形状 Provider：每种形状一个实例
    static ShapeComponentProvider* s_pRect = new ShapeComponentProvider(E_SHAPE_TYPE_RECTANGLE, 0);
    static ShapeComponentProvider* s_pRoundRect = new ShapeComponentProvider(E_SHAPE_TYPE_ROUND_RECT, 1);
    static ShapeComponentProvider* s_pEllipse = new ShapeComponentProvider(E_SHAPE_TYPE_ELLIPSE, 2);
    static ShapeComponentProvider* s_pLine = new ShapeComponentProvider(E_SHAPE_TYPE_LINE, 3);

    // 贴纸 Provider：每种贴纸一个实例
    static StickerComponentProvider* s_pTitleLine = new StickerComponentProvider(E_STICKER_TYPE_TITLE_LINE, 0);
    static StickerComponentProvider* s_pCornerBadge = new StickerComponentProvider(E_STICKER_TYPE_CORNER_BADGE, 1);
    static StickerComponentProvider* s_pStarRating = new StickerComponentProvider(E_STICKER_TYPE_STAR_RATING, 2);
    static StickerComponentProvider* s_pArrow = new StickerComponentProvider(E_STICKER_TYPE_ARROW, 3);
    static StickerComponentProvider* s_pDivider = new StickerComponentProvider(E_STICKER_TYPE_DIVIDER, 4);
    static StickerComponentProvider* s_pCardBorder = new StickerComponentProvider(E_STICKER_TYPE_CARD_BORDER, 5);

    pHost->registerComponentProvider(s_pImage);
    pHost->registerComponentProvider(s_pText);
    pHost->registerComponentProvider(s_pTable);
    pHost->registerComponentProvider(s_pRect);
    pHost->registerComponentProvider(s_pRoundRect);
    pHost->registerComponentProvider(s_pEllipse);
    pHost->registerComponentProvider(s_pLine);
    pHost->registerComponentProvider(s_pTitleLine);
    pHost->registerComponentProvider(s_pCornerBadge);
    pHost->registerComponentProvider(s_pStarRating);
    pHost->registerComponentProvider(s_pArrow);
    pHost->registerComponentProvider(s_pDivider);
    pHost->registerComponentProvider(s_pCardBorder);

    // ---- 导出格式 Provider ----
    static PngSeparateExportProvider* s_pPngSeparate = new PngSeparateExportProvider;
    static PngLongImageExportProvider* s_pPngLongImage = new PngLongImageExportProvider;
    pHost->registerExportProvider(s_pPngSeparate);
    pHost->registerExportProvider(s_pPngLongImage);

    // ---- 模板 Provider ----
    static BuiltinTemplateProvider* s_pTemplate = new BuiltinTemplateProvider;
    pHost->registerTemplateProvider(s_pTemplate);

    // ---- 主题 Provider ----
    static BuiltinThemeProvider* s_pTheme = new BuiltinThemeProvider;
    pHost->registerThemeProvider(s_pTheme);
}

} // namespace bwm
