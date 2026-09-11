/**
 * @file GlassStyleProvider.cpp
 * @author zhangweimu
 * @brief 内置玻璃拟态 UI 风格 Provider 实现。
 *
 * 玻璃观感由三层叠加而成：
 * 1. 窗口渐变底（paintBackground）：对角线性渐变，带 alpha，营造氛围色；
 * 2. 半透明面板 + 亮色内边框（buildStyleSheet）：模拟玻璃片浮起的效果；
 * 3. Windows DWM 亚克力（AcrylicHelper）：桌面模糊透过半透明渐变形成层次。
 *
 * 第 1、2 层不依赖平台，故玻璃观感在非 Windows 上同样生效；第 3 层仅 Windows
 * 且属增强项，调用失败不阻断（QSS + 渐变已足以呈现玻璃观感）。
 *
 * 技术限制说明：Qt Widgets 的 QSS 不支持 backdrop-filter，无法真正模糊面板背后
 * 的窗口内容（那是网页端毛玻璃的做法）。这里用"彩色渐变 + 半透明面板 + 亮色
 * 内边框"近似玻璃拟态——因为背景是平滑渐变，缺少模糊几乎不影响观感。
 */
#include "ui/GlassStyleProvider.h"

#include "ui/AcrylicHelper.h"
#include "ui/UiStyle.h"

#include <QGuiApplication>
#include <QLinearGradient>
#include <QPainter>
#include <QStyleHints>
#include <QWidget>

namespace bwm {

namespace {

/**
 * @brief 玻璃底渐变的三段端点色与整体不透明度。
 */
struct GlassGradient {
    QColor cTopLeft;      ///< 左上：氛围主色
    QColor cMiddle;       ///< 中部：过渡色
    QColor cBottomRight;  ///< 右下：收暗（深色）/ 收亮（浅色）
    int nAlpha;           ///< 整体不透明度，< 255 让 DWM 桌面模糊透出层次
};

// 深色玻璃：深紫 → 深蓝 → 墨黑
const GlassGradient& darkGradient()
{
    static const GlassGradient s_gradient{
        QColor(30, 27, 56), QColor(21, 32, 61), QColor(10, 12, 18), 200
    };
    return s_gradient;
}

// 浅色玻璃：奶白 → 浅蓝 → 浅灰
const GlassGradient& lightGradient()
{
    static const GlassGradient s_gradient{
        QColor(250, 251, 254), QColor(228, 238, 252), QColor(242, 245, 250), 232
    };
    return s_gradient;
}

/**
 * @brief 当前系统是否处于深色模式。
 *
 * 使用 Qt 6.5+ 的 QStyleHints::colorScheme()，跨平台且无需读注册表。
 * 系统未提供配色信息（Qt::ColorScheme::Unknown）时按浅色处理。
 */
bool isSystemDark()
{
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

/**
 * @brief 把界面外观 id 归一化为深/浅两种具体外观。
 *
 * 「跟随系统」在此处按当前系统配色解析，因此后续绘制与样式表逻辑
 * 只需处理深/浅两种分支。
 *
 * @param strStyleId  风格 id
 * @param rbDark      [out] true 表示深色外观，false 表示浅色外观
 * @return 是否属于本 Provider 的风格（false 表示不处理该 id）
 */
bool normalizeStyleId(const QString& strStyleId, bool& rbDark)
{
    if(strStyleId == UiStyleManager::kDarkId) {
        rbDark = true;
        return true;
    }
    if(strStyleId == UiStyleManager::kLightId) {
        rbDark = false;
        return true;
    }
    if(strStyleId == UiStyleManager::kAutoId) {
        rbDark = isSystemDark();
        return true;
    }
    return false;
}

/**
 * @brief 生成玻璃拟态样式表。
 *
 * 要点：
 * - 容器（主窗口/分隔器/堆叠容器/工具栏/菜单栏/状态栏）背景透明，让窗口渐变透上来；
 * - 面板用半透明白色浮层 + 亮色内边框，形成"玻璃片浮起"的观感；
 * - 对话框是独立窗口、没有渐变底，故给它们自带的不透明玻璃底色，避免透出桌面；
 * - 强调色统一为偏青绿的 rgba(74, 158, 142, x)。
 *
 * @param bDark  true 为深色玻璃（浅色文字），false 为浅色玻璃（深色文字）
 */
QString buildStyleSheet(bool bDark)
{
    if (bDark) {
        return QStringLiteral(R"CSS(
/* ---- 容器透明：让窗口渐变底透上来 ---- */
QMainWindow, QSplitter, QStackedWidget, QToolBar, QMenuBar, QStatusBar {
    background: transparent;
    border: none;
}

/* ---- 对话框：独立窗口无渐变底，自带不透明玻璃底 ---- */
QDialog, QMessageBox {
    background-color: rgba(26, 28, 40, 250);
    color: #E4E6EB;
}

/* ---- 菜单 ---- */
QMenuBar { color: #E4E6EB; }
QMenuBar::item { background: transparent; padding: 4px 10px; border-radius: 6px; }
QMenuBar::item:selected { background-color: rgba(255, 255, 255, 32); }
QMenu {
    background-color: rgba(34, 36, 50, 242);
    color: #E4E6EB;
    border: 1px solid rgba(255, 255, 255, 40);
    border-radius: 8px;
    padding: 4px;
}
QMenu::item { padding: 5px 22px; border-radius: 6px; }
QMenu::item:selected { background-color: rgba(74, 158, 142, 150); color: #FFFFFF; }
QMenu::separator { height: 1px; background: rgba(255, 255, 255, 30); margin: 4px 8px; }

/* ---- 工具栏 / 状态栏 ---- */
QToolBar { spacing: 4px; }
QStatusBar { color: #B9BEC9; }

/* ---- 面板：半透明白浮层 + 亮色内边框 = 玻璃片 ---- */
QTreeView, QListView {
    background-color: rgba(255, 255, 255, 16);
    color: #E4E6EB;
    border: 1px solid rgba(255, 255, 255, 34);
    border-radius: 8px;
    outline: none;
}
QTreeView::item, QListView::item { padding: 3px; border-radius: 5px; }
QTreeView::item:hover, QListView::item:hover { background-color: rgba(255, 255, 255, 22); }
QTreeView::item:selected, QListView::item:selected {
    background-color: rgba(74, 158, 142, 150);
    color: #FFFFFF;
}

/* ---- 标签页 ---- */
QTabWidget::pane { background: transparent; border: none; }
QTabBar::tab {
    background-color: rgba(255, 255, 255, 14);
    color: #C7CCD6;
    padding: 6px 16px;
    margin: 2px;
    border: 1px solid rgba(255, 255, 255, 30);
    border-radius: 8px;
}
QTabBar::tab:selected {
    background-color: rgba(74, 158, 142, 140);
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 60);
}

/* ---- 文本编辑 / 输入控件 ---- */
QTextEdit, QPlainTextEdit {
    background-color: rgba(255, 255, 255, 22);
    color: #E4E6EB;
    border: 1px solid rgba(255, 255, 255, 34);
    border-radius: 8px;
}
QLineEdit, QSpinBox, QComboBox {
    background-color: rgba(255, 255, 255, 22);
    color: #E4E6EB;
    border: 1px solid rgba(255, 255, 255, 34);
    border-radius: 6px;
    padding: 4px 6px;
}
QPushButton {
    background-color: rgba(255, 255, 255, 26);
    color: #E4E6EB;
    border: 1px solid rgba(255, 255, 255, 40);
    border-radius: 6px;
    padding: 5px 14px;
}
QPushButton:hover { background-color: rgba(255, 255, 255, 44); }
QPushButton:pressed { background-color: rgba(74, 158, 142, 150); color: #FFFFFF; }
QLabel { color: #E4E6EB; background: transparent; }
QHeaderView::section {
    background-color: rgba(255, 255, 255, 16);
    color: #E4E6EB;
    border: none;
    padding: 4px;
}

/* ---- 滚动条 / 分隔条 ---- */
QScrollBar:vertical, QScrollBar:horizontal { background: transparent; border: none; margin: 0; }
QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background-color: rgba(255, 255, 255, 60);
    border-radius: 4px;
    min-height: 24px;
    min-width: 24px;
}
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
QSplitter::handle { background-color: rgba(255, 255, 255, 18); }
)CSS");
    }

    // 浅色玻璃：深色文字 + 更实的白色浮层
    return QStringLiteral(R"CSS(
QMainWindow, QSplitter, QStackedWidget, QToolBar, QMenuBar, QStatusBar {
    background: transparent;
    border: none;
}
QDialog, QMessageBox {
    background-color: rgba(248, 250, 253, 252);
    color: #1F2430;
}
QMenuBar { color: #1F2430; }
QMenuBar::item { background: transparent; padding: 4px 10px; border-radius: 6px; }
QMenuBar::item:selected { background-color: rgba(255, 255, 255, 180); }
QMenu {
    background-color: rgba(252, 253, 255, 248);
    color: #1F2430;
    border: 1px solid rgba(255, 255, 255, 220);
    border-radius: 8px;
    padding: 4px;
}
QMenu::item { padding: 5px 22px; border-radius: 6px; }
QMenu::item:selected { background-color: rgba(74, 158, 142, 160); color: #FFFFFF; }
QMenu::separator { height: 1px; background: rgba(0, 0, 0, 30); margin: 4px 8px; }
QToolBar { spacing: 4px; }
QStatusBar { color: #4A5262; }
QTreeView, QListView {
    background-color: rgba(255, 255, 255, 150);
    color: #1F2430;
    border: 1px solid rgba(255, 255, 255, 210);
    border-radius: 8px;
    outline: none;
}
QTreeView::item, QListView::item { padding: 3px; border-radius: 5px; }
QTreeView::item:hover, QListView::item:hover { background-color: rgba(255, 255, 255, 215); }
QTreeView::item:selected, QListView::item:selected {
    background-color: rgba(74, 158, 142, 160);
    color: #FFFFFF;
}
QTabWidget::pane { background: transparent; border: none; }
QTabBar::tab {
    background-color: rgba(255, 255, 255, 140);
    color: #3A4250;
    padding: 6px 16px;
    margin: 2px;
    border: 1px solid rgba(255, 255, 255, 190);
    border-radius: 8px;
}
QTabBar::tab:selected {
    background-color: rgba(74, 158, 142, 150);
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 230);
}
QTextEdit, QPlainTextEdit {
    background-color: rgba(255, 255, 255, 175);
    color: #1F2430;
    border: 1px solid rgba(255, 255, 255, 210);
    border-radius: 8px;
}
QLineEdit, QSpinBox, QComboBox {
    background-color: rgba(255, 255, 255, 185);
    color: #1F2430;
    border: 1px solid rgba(0, 0, 0, 32);
    border-radius: 6px;
    padding: 4px 6px;
}
QPushButton {
    background-color: rgba(255, 255, 255, 195);
    color: #1F2430;
    border: 1px solid rgba(0, 0, 0, 38);
    border-radius: 6px;
    padding: 5px 14px;
}
QPushButton:hover { background-color: rgba(255, 255, 255, 240); }
QPushButton:pressed { background-color: rgba(74, 158, 142, 150); color: #FFFFFF; }
QLabel { color: #1F2430; background: transparent; }
QHeaderView::section {
    background-color: rgba(255, 255, 255, 150);
    color: #1F2430;
    border: none;
    padding: 4px;
}
QScrollBar:vertical, QScrollBar:horizontal { background: transparent; border: none; margin: 0; }
QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background-color: rgba(0, 0, 0, 70);
    border-radius: 4px;
    min-height: 24px;
    min-width: 24px;
}
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
QSplitter::handle { background-color: rgba(255, 255, 255, 150); }
)CSS");
}

} // namespace

QString GlassStyleProvider::providerId() const
{
    return QStringLiteral("builtin-ui");
}

QVector<UiStyleDescriptor> GlassStyleProvider::styles() const
{
    QVector<UiStyleDescriptor> vecStyles;
    // bRequiresNativeApi 一律置 false：玻璃观感由 QSS + 渐变实现，全平台可用
    //（Windows 上会额外叠加 DWM 亚克力增强，但非必需）。
    vecStyles.append({UiStyleManager::kDarkId, QStringLiteral("深色"), false});
    vecStyles.append({UiStyleManager::kLightId, QStringLiteral("浅色"), false});
    vecStyles.append({UiStyleManager::kAutoId, QStringLiteral("跟随系统"), false});
    return vecStyles;
}

bool GlassStyleProvider::applyStyle(const QString& strStyleId, QWidget* pMainWindow) const
{
    if(!pMainWindow) {
        return false;
    }
    // 归一化为深/浅外观（「跟随系统」在此按系统配色解析）
    bool bDark = false;
    if(!normalizeStyleId(strStyleId, bDark)) {
        return false;
    }

    // 渐变底与玻璃面板样式表在全平台生效（不依赖平台原生 API）
    pMainWindow->setStyleSheet(buildStyleSheet(bDark));

#ifdef Q_OS_WIN
    // Windows 上额外启用 DWM 亚克力，让桌面模糊透过带 alpha 的渐变形成层次。
    // 这里是增强而非必需：调用失败也不影响 QSS + 渐变呈现的玻璃观感。
    const GlassGradient& rGradient = bDark ? darkGradient() : lightGradient();
    QColor tint = rGradient.cTopLeft;
    tint.setAlpha(rGradient.nAlpha);
    AcrylicHelper::enableAcrylic(pMainWindow, bDark, tint);
#else
    // TODO: 非 Windows 平台可选接入 QSS 模拟亚克力（如叠加噪点/高光层）。
    //       当前渐变 + 半透明面板已能在全平台呈现玻璃拟态，故不返回 false 触发回退。
#endif
    return true;
}

bool GlassStyleProvider::paintBackground(const QString& strStyleId, QWidget* pMainWindow,
                                        QPainter& rPainter) const
{
    if(!pMainWindow) {
        return false;
    }
    // 归一化为深/浅外观；非本 Provider 的风格返回 false，交给 Qt 默认绘制
    bool bDark = false;
    if(!normalizeStyleId(strStyleId, bDark)) {
        return false;
    }
    const GlassGradient& rGlass = bDark ? darkGradient() : lightGradient();

    // 对角线性渐变：左上氛围色 → 中部过渡色 → 右下收暗/收亮。
    // 整体带 alpha，使 Windows DWM 的桌面模糊能透出，形成层次感。
    QLinearGradient gradient(0.0, 0.0,
                             static_cast<qreal>(pMainWindow->width()),
                             static_cast<qreal>(pMainWindow->height()));
    QColor cTopLeft = rGlass.cTopLeft;
    QColor cMiddle = rGlass.cMiddle;
    QColor cBottomRight = rGlass.cBottomRight;
    cTopLeft.setAlpha(rGlass.nAlpha);
    cMiddle.setAlpha(rGlass.nAlpha);
    cBottomRight.setAlpha(rGlass.nAlpha);
    gradient.setColorAt(0.0, cTopLeft);
    gradient.setColorAt(0.45, cMiddle);
    gradient.setColorAt(1.0, cBottomRight);

    rPainter.fillRect(pMainWindow->rect(), gradient);
    return true;
}

QColor GlassStyleProvider::canvasBackgroundColor(const QString& strStyleId) const
{
    bool bDark = false;
    if(!normalizeStyleId(strStyleId, bDark)) {
        // 非本 Provider 的风格：返回无效色表示不干预，由调用方恢复画布默认背景
        return QColor();
    }
    // 玻璃风格：画布视口完全透明，让窗口渐变透上来，页面像"浮"在玻璃上。
    // 页面本身由 CanvasScene 绘制，仍保持完整对比度。
    return QColor(0, 0, 0, 0);
}

} // namespace bwm
