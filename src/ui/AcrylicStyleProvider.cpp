/**
 * @file AcrylicStyleProvider.cpp
 * @author zhangweimu
 * @brief 内置亚克力风格 Provider 实现。
 *
 * 深浅色差异通过两层实现：
 * 1. DWM 原生亚克力背景（Windows 10/11），immersive dark mode 控制标题栏深浅。
 * 2. 半透明面板 stylesheet：面板背景用 rgba() 半透明色，让 DWM 模糊透上来，
 *    文字色/边框色跟随深浅。Win11 下 backdrop tint 由系统控制无法自定义，
 *    靠 stylesheet 的半透明 tint 叠加实现深浅差异。
 */
#include "ui/AcrylicStyleProvider.h"

#include "ui/AcrylicHelper.h"
#include "ui/UiStyle.h"

#include <QWidget>

namespace bwm {

namespace {

/**
 * @brief 生成半透明面板 stylesheet，让 DWM 模糊透上来 + 文字色跟随深浅。
 *
 * 不覆盖 QGraphicsView（画布），画布背景由 CanvasView/Theme 控制。
 * 主窗口整体用 rgba(30,30,35,180) 半透明底（深色）或 rgba(243,243,243,180)（浅色），
 * 让 DWM 模糊透上来约 70% 不透明度，tint 叠加实现深浅差异。
 */
QString buildStyleSheet(bool bDark)
{
    if (bDark) {
        return QStringLiteral(
            "QMainWindow {"
            "  background-color: rgba(30, 30, 35, 180);"
            "  color: #DCDCDC;"
            "}"
            "QTreeView, QListView {"
            "  background-color: rgba(30, 30, 35, 180);"
            "  color: #DCDCDC;"
            "  border: 1px solid rgba(255, 255, 255, 40);"
            "  outline: none;"
            "}"
            "QTreeView::item:hover, QListView::item:hover {"
            "  background-color: rgba(255, 255, 255, 20);"
            "}"
            "QTreeView::item:selected, QListView::item:selected {"
            "  background-color: rgba(69, 123, 157, 180);"
            "  color: #FFFFFF;"
            "}"
            "QTabWidget::pane {"
            "  background-color: rgba(30, 30, 35, 180);"
            "  border: 1px solid rgba(255, 255, 255, 40);"
            "}"
            "QTabBar::tab {"
            "  background-color: rgba(30, 30, 35, 180);"
            "  color: #DCDCDC;"
            "  padding: 6px 16px;"
            "  border: 1px solid rgba(255, 255, 255, 40);"
            "}"
            "QTabBar::tab:selected {"
            "  background-color: rgba(69, 123, 157, 180);"
            "  color: #FFFFFF;"
            "}"
            "QToolBar {"
            "  background-color: rgba(30, 30, 35, 180);"
            "  border: none;"
            "  color: #DCDCDC;"
            "}"
            "QStatusBar {"
            "  background-color: rgba(30, 30, 35, 180);"
            "  color: #DCDCDC;"
            "}"
            "QMenuBar {"
            "  background-color: rgba(30, 30, 35, 180);"
            "  color: #DCDCDC;"
            "  border: none;"
            "}"
            "QMenuBar::item:selected {"
            "  background-color: rgba(69, 123, 157, 180);"
            "}"
            "QMenu {"
            "  background-color: rgba(40, 40, 46, 230);"
            "  color: #DCDCDC;"
            "  border: 1px solid rgba(255, 255, 255, 40);"
            "}"
        );
    }
    // 浅色亚克力
    return QStringLiteral(
        "QMainWindow {"
        "  background-color: rgba(243, 243, 243, 180);"
        "  color: #212121;"
        "}"
        "QTreeView, QListView {"
        "  background-color: rgba(243, 243, 243, 180);"
        "  color: #212121;"
        "  border: 1px solid rgba(0, 0, 0, 40);"
        "  outline: none;"
        "}"
        "QTreeView::item:hover, QListView::item:hover {"
        "  background-color: rgba(0, 0, 0, 20);"
        "}"
        "QTreeView::item:selected, QListView::item:selected {"
        "  background-color: rgba(69, 123, 157, 180);"
        "  color: #FFFFFF;"
        "}"
        "QTabWidget::pane {"
        "  background-color: rgba(243, 243, 243, 180);"
        "  border: 1px solid rgba(0, 0, 0, 40);"
        "}"
        "QTabBar::tab {"
        "  background-color: rgba(243, 243, 243, 180);"
        "  color: #212121;"
        "  padding: 6px 16px;"
        "  border: 1px solid rgba(0, 0, 0, 40);"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: rgba(69, 123, 157, 180);"
        "  color: #FFFFFF;"
        "}"
        "QToolBar {"
        "  background-color: rgba(243, 243, 243, 180);"
        "  border: none;"
        "  color: #212121;"
        "}"
        "QStatusBar {"
        "  background-color: rgba(243, 243, 243, 180);"
        "  color: #212121;"
        "}"
        "QMenuBar {"
        "  background-color: rgba(243, 243, 243, 180);"
        "  color: #212121;"
        "  border: none;"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: rgba(69, 123, 157, 180);"
        "}"
        "QMenu {"
        "  background-color: rgba(250, 250, 252, 230);"
        "  color: #212121;"
        "  border: 1px solid rgba(0, 0, 0, 40);"
        "}"
    );
}

} // namespace

QString AcrylicStyleProvider::providerId() const
{
    return QStringLiteral("builtin-ui");
}

QVector<UiStyleDescriptor> AcrylicStyleProvider::styles() const
{
    QVector<UiStyleDescriptor> vecStyles;
    // system 不在此提供——它仅作为运行时 fallback（非 Windows 降级），不暴露给用户
    vecStyles.append({
        UiStyleManager::kAcrylicDarkId,
        QStringLiteral("深色亚克力"),
        true   // 依赖 Windows 原生 API
    });
    vecStyles.append({
        UiStyleManager::kAcrylicLightId,
        QStringLiteral("浅色亚克力"),
        true   // 依赖 Windows 原生 API
    });
    return vecStyles;
}

bool AcrylicStyleProvider::applyStyle(const QString& strStyleId, QWidget* pMainWindow) const
{
    if(strStyleId == UiStyleManager::kSystemId) {
        // 系统默认：禁用 DWM 亚克力 + 清除样式表恢复 Fusion 默认外观
        AcrylicHelper::disable(pMainWindow);
        pMainWindow->setStyleSheet(QString());
        return true;
    }
#ifdef Q_OS_WIN
    if(strStyleId == UiStyleManager::kAcrylicDarkId) {
        if(!AcrylicHelper::enableAcrylic(pMainWindow, /*bDark=*/true,
                                          QColor(32, 32, 40, 200))) {
            return false;
        }
        pMainWindow->setStyleSheet(buildStyleSheet(/*bDark=*/true));
        return true;
    }
    if(strStyleId == UiStyleManager::kAcrylicLightId) {
        if(!AcrylicHelper::enableAcrylic(pMainWindow, /*bDark=*/false,
                                          QColor(243, 243, 243, 200))) {
            return false;
        }
        pMainWindow->setStyleSheet(buildStyleSheet(/*bDark=*/false));
        return true;
    }
#else
    // TODO: 非 Windows 平台用 QSS 模拟亚克力观感。当前仅占位，返回 false 由
    //       UiStyleManager 回退到 system。
    Q_UNUSED(strStyleId)
    Q_UNUSED(pMainWindow)
#endif
    return false;
}

} // namespace bwm
