/**
 * @file Settings.h
 * @author zhangweimu
 * @brief 全局设置（QSettings 持久化）的读写接口。
 */
#ifndef BWM_SETTINGS_SETTINGS_H
#define BWM_SETTINGS_SETTINGS_H

#include <QByteArray>
#include <QSize>
#include <QString>
#include <QStringList>

#include "core/AuthorMarkStyle.h"

class QSettings;

namespace bwm {

// 全局设置（QSettings 持久化，规划第 5.8 节）。全部为静态接口，任何模块可直接调用。
class Settings {
public:
    // 默认画布尺寸（新建项目向导使用）
    static QSize defaultPageSize();
    static void setDefaultPageSize(const QSize& size);

    // 自动保存间隔（毫秒）
    static int autoSaveIntervalMs();
    static void setAutoSaveIntervalMs(int nIntervalMs);

    // 作者署名（如小黑盒 ID）；导出时可选应用，M4 使用
    static QString authorName();
    static void setAuthorName(const QString& strName);

    // 署名水印样式（位置/字体/字号/加粗/颜色/不透明度）；由导出对话框「署名设置…」修改。
    // 未设置过时返回默认样式（等价于旧版本硬编码：右下角、微软雅黑 18px、黑色 63%）。
    static AuthorMarkStyle authorMarkStyle();
    static void setAuthorMarkStyle(const AuthorMarkStyle& rStyle);

    // 最近项目（json 路径列表，最近优先）
    static QStringList recentProjects();
    static void setRecentProjects(const QStringList& vecPaths);

    // 当前画布配色主题名（内置主题见 theme/Theme.h）
    static QString themeName();
    static void setThemeName(const QString& rName);

    // 当前 UI 风格 id（应用窗口/控件外观，见 ui/UiStyle.h）
    static QString uiStyle();
    static void setUiStyle(const QString& rId);

    // 主窗口几何状态（关闭时保存，启动时恢复）
    static QByteArray windowGeometry();
    static void setWindowGeometry(const QByteArray& rGeometry);

    // 上次导出目录（导出对话框默认填充，导出成功后更新）
    static QString lastExportDirectory();
    static void setLastExportDirectory(const QString& strDir);

private:
    static QSettings& settings();
};

} // namespace bwm

#endif // BWM_SETTINGS_SETTINGS_H
