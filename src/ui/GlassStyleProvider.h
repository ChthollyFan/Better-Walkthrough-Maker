/**
 * @file GlassStyleProvider.h
 * @author zhangweimu
 * @brief 内置玻璃拟态 UI 风格 Provider。
 *
 * 提供三种界面外观：深色 / 浅色 / 跟随系统。
 * - 「跟随系统」按当前系统配色（QStyleHints::colorScheme）解析为深色或浅色；
 * - 玻璃观感由三层叠加：窗口渐变底 + 半透明面板 +（Windows）DWM 亚克力增强。
 *
 * 前两层不依赖平台，因此玻璃观感在非 Windows 上同样生效；
 * DWM 亚克力仅 Windows 且属增强项，调用失败不影响观感。
 */
#ifndef BWM_UI_GLASSSTYLEPROVIDER_H
#define BWM_UI_GLASSSTYLEPROVIDER_H

#include "plugin/IUiStyleProvider.h"

namespace bwm {

/**
 * @brief 内置玻璃拟态风格 Provider。
 *
 * 新增内置界面外观时，只需在 styles() 追加一项、并让 normalizeStyleId()
 * 认识该 id，菜单会自动出现新选项，无需修改框架其他部分。
 */
class GlassStyleProvider : public IUiStyleProvider
{
public:
    QString providerId() const override;
    QVector<UiStyleDescriptor> styles() const override;
    bool applyStyle(const QString& strStyleId, QWidget* pMainWindow) const override;

    // 绘制窗口渐变底（玻璃风格的氛围色层）
    bool paintBackground(const QString& strStyleId, QWidget* pMainWindow,
                         QPainter& rPainter) const override;

    // 玻璃风格下把画布视口设为全透明，让窗口渐变透上来
    QColor canvasBackgroundColor(const QString& strStyleId) const override;
};

} // namespace bwm

#endif // BWM_UI_GLASSSTYLEPROVIDER_H
