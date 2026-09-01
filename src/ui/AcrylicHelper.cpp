/**
 * @file AcrylicHelper.cpp
 * @author zhangweimu
 * @brief Windows 原生亚克力效果实现。
 *
 * 通过 DWM API 启用窗口背景模糊。Windows 11 使用 SYSTEMBACKDROP_TYPE，
 * Windows 10 回退到 SetWindowCompositionAttribute + ACCENT_ENABLE_ACRYLICBLURBEHIND。
 * 非 Windows 平台为空实现。
 */
#include "ui/AcrylicHelper.h"

#include <QWidget>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>

// RtlGetVersion 声明在 winternl.h，这里手动声明避免引入额外头文件
typedef LONG (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
// DWMSBT_AUTO=0, DWMSBT_NONE=1, DWMSBT_TRANSIENTWINDOW(亚克力)=2, DWMSBT_MAINWINDOW(云母)=3
#define DWMSBT_TRANSIENTWINDOW 2
#define DWMSBT_AUTO 0
#define DWMSBT_NONE 1

// Windows 10 ACCENTPOLICY 相关定义（用于 SetWindowCompositionAttribute）
#ifndef ACCENT_ENABLE_ACRYLICBLURBEHIND
#define ACCENT_ENABLE_ACRYLICBLURBEHIND 4
#endif
#ifndef WCA_ACCENT_POLICY
#define WCA_ACCENT_POLICY 19
#endif

namespace {

// Win10 窗口合成属性结构
struct ACCENTPOLICY {
    int nAccentState;
    int nFlags;
    DWORD nColor;       // ABGR
    int nAnimationId;
};

struct WINCOMPATTRDATA {
    int nAttribute;
    PVOID pData;
    ULONG ulDataSize;
};

typedef BOOL (WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);

// 判断是否 Windows 11（Build >= 22000）
bool isWindows11()
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll");
    if(!hNtdll) {
        return false;
    }
    auto pRtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(
        reinterpret_cast<void*>(GetProcAddress(hNtdll, "RtlGetVersion")));
    if(!pRtlGetVersion) {
        return false;
    }
    RTL_OSVERSIONINFOW info{};
    info.dwOSVersionInfoSize = sizeof(info);
    if(pRtlGetVersion(&info) != 0) {
        return false;
    }
    return info.dwMajorVersion == 10 && info.dwBuildNumber >= 22000;
}

// Win10 路径：用 SetWindowCompositionAttribute 启用亚克力模糊
bool enableAcrylicWin10(HWND hwnd, const QColor& rTint)
{
    HMODULE hUser32 = GetModuleHandleW(L"user32");
    if(!hUser32) {
        return false;
    }
    auto pFn = reinterpret_cast<pSetWindowCompositionAttribute>(
        reinterpret_cast<void*>(GetProcAddress(hUser32, "SetWindowCompositionAttribute")));
    if(!pFn) {
        return false;
    }

    // tint 颜色为 ABGR（注意与 ARGB 的差异）
    const DWORD abgr = (rTint.alpha() << 24)
                     | (rTint.blue() << 16)
                     | (rTint.green() << 8)
                     | rTint.red();

    ACCENTPOLICY policy{};
    policy.nAccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
    policy.nFlags = 0;
    policy.nColor = abgr;
    policy.nAnimationId = 0;

    WINCOMPATTRDATA data{};
    data.nAttribute = WCA_ACCENT_POLICY;
    data.pData = &policy;
    data.ulDataSize = sizeof(policy);

    return pFn(hwnd, &data) != FALSE;
}

} // namespace

#endif // Q_OS_WIN

namespace bwm {

bool AcrylicHelper::enableAcrylic(QWidget* pWin, bool bDark, const QColor& rTint)
{
#ifdef Q_OS_WIN
    if(!pWin || !pWin->winId()) {
        return false;
    }
    // 确保有原生窗口句柄
    const HWND hwnd = reinterpret_cast<HWND>(pWin->winId());

    // 让窗口支持透明背景，DWM 模糊才能透上来
    pWin->setAttribute(Qt::WA_TranslucentBackground);

    // 沉浸式深色标题栏
    const BOOL bDarkMode = bDark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE,
                          &bDarkMode, sizeof(bDarkMode));

    if(isWindows11()) {
        // Win11: 用 SYSTEMBACKDROP，亚克力 = DWMSBT_TRANSIENTWINDOW
        int nBackdrop = DWMSBT_TRANSIENTWINDOW;
        if(DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE,
                                 &nBackdrop, sizeof(nBackdrop)) == S_OK) {
            return true;
        }
        // SYSTEMBACKDROP 失败则回退到 Win10 路径
    }

    // Win10 路径
    return enableAcrylicWin10(hwnd, rTint);
#else
    Q_UNUSED(pWin)
    Q_UNUSED(bDark)
    Q_UNUSED(rTint)
    // TODO: 非 Windows 平台用 QSS 模拟亚克力观感。当前仅占位。
    return false;
#endif
}

void AcrylicHelper::disable(QWidget* pWin)
{
#ifdef Q_OS_WIN
    if(!pWin || !pWin->winId()) {
        return;
    }
    const HWND hwnd = reinterpret_cast<HWND>(pWin->winId());

    // 禁用 SYSTEMBACKDROP（AUTO 在 Win11 下仍可能自动应用亚克力，必须用 NONE）
    int nBackdrop = DWMSBT_NONE;
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE,
                          &nBackdrop, sizeof(nBackdrop));

    // 取消透明背景属性
    pWin->setAttribute(Qt::WA_TranslucentBackground, false);
#else
    Q_UNUSED(pWin)
#endif
}

} // namespace bwm
