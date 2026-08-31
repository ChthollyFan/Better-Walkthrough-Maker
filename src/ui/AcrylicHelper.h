/**
 * @file AcrylicHelper.h
 * @author zhangweimu
 * @brief Windows 原生亚克力（Acrylic）效果辅助工具。
 *
 * 封装 DWM API，为主窗口启用/禁用亚克力半透明模糊背景。
 * - Windows 11（Build >= 22000）：DwmSetWindowAttribute + SYSTEMBACKDROP_TYPE
 * - Windows 10：SetWindowCompositionAttribute + ACCENT_ENABLE_ACRYLICBLURBEHIND
 * - 非 Windows：空实现，所有方法返回 false（由框架回退到 system 风格）
 *
 * TODO: 非 Windows 平台用 QSS 模拟亚克力观感（半透明色块 + 圆角 + 边框）。
 *       当前仅留接口占位，实际 QSS 模拟未实现。
 */
#ifndef BWM_UI_ACRYLICHELPER_H
#define BWM_UI_ACRYLICHELPER_H

#include <QColor>

class QWidget;

namespace bwm {

/**
 * @brief 亚克力效果辅助工具（静态方法集）。
 */
class AcrylicHelper
{
public:
    /**
     * @brief 为主窗口启用亚克力模糊背景。
     * @param pWin  目标窗口（需已创建原生窗口句柄）
     * @param bDark  是否深色模式（影响标题栏沉浸式深色）
     * @param rTint  混合底色（含 alpha，仅 Win10 路径使用）
     * @return 成功返回 true；非 Windows 或窗口无效返回 false
     */
    static bool enableAcrylic(QWidget* pWin, bool bDark, const QColor& rTint);

    /**
     * @brief 禁用亚克力，还原系统默认窗口背景。
     * @param pWin  目标窗口
     */
    static void disable(QWidget* pWin);
};

} // namespace bwm

#endif // BWM_UI_ACRYLICHELPER_H
