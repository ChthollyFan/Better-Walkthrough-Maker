/**
 * @file AcrylicStyleProvider.h
 * @author zhangweimu
 * @brief 内置亚克力 UI 风格 Provider。
 *
 * 提供 acrylic-dark / acrylic-light 两种亚克力风格。system 仅作运行时 fallback
 * （非 Windows 降级），不在此提供。
 * 亚克力风格通过 AcrylicHelper 调用 Windows DWM API 实现真实半透明模糊；
 * 非 Windows 平台 applyStyle 返回 false，由 UiStyleManager 回退到 system。
 */
#ifndef BWM_UI_ACRYLICSTYLEPROVIDER_H
#define BWM_UI_ACRYLICSTYLEPROVIDER_H

#include "plugin/IUiStyleProvider.h"

namespace bwm {

/**
 * @brief 内置亚克力风格 Provider。
 *
 * 新增内置 UI 风格时，只需在 styles() 追加一项、applyStyle() 加一个 if 分支，
 * 菜单会自动出现新选项，无需修改框架其他部分。
 */
class AcrylicStyleProvider : public IUiStyleProvider
{
public:
    QString providerId() const override;
    QVector<UiStyleDescriptor> styles() const override;
    bool applyStyle(const QString& strStyleId, QWidget* pMainWindow) const override;
};

} // namespace bwm

#endif // BWM_UI_ACRYLICSTYLEPROVIDER_H
