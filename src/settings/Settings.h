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

    // 最近项目（json 路径列表，最近优先）
    static QStringList recentProjects();
    static void setRecentProjects(const QStringList& vecPaths);

    // 当前主题名（内置主题见 theme/Theme.h）
    static QString themeName();
    static void setThemeName(const QString& rName);

    // 主窗口几何状态（关闭时保存，启动时恢复）
    static QByteArray windowGeometry();
    static void setWindowGeometry(const QByteArray& rGeometry);

private:
    static QSettings& settings();
};

} // namespace bwm

#endif // BWM_SETTINGS_SETTINGS_H
