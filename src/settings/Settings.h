#pragma once

#include <QSize>
#include <QString>
#include <QStringList>

class QSettings;

namespace bwm {

// 全局设置（QSettings 持久化，规划第 5.8 节）。
// 全部为静态接口，任何模块可直接调用。
class Settings {
public:
    // 默认画布尺寸（新建项目向导使用）
    static QSize defaultPageSize();
    static void setDefaultPageSize(const QSize &size);

    // 自动保存间隔（毫秒）
    static int autoSaveIntervalMs();
    static void setAutoSaveIntervalMs(int intervalMs);

    // 作者署名（如小黑盒 ID）；导出时可选应用，M4 使用
    static QString authorName();
    static void setAuthorName(const QString &name);

    // 最近项目（json 路径列表，最近优先）
    static QStringList recentProjects();
    static void setRecentProjects(const QStringList &paths);

private:
    static QSettings &settings();
};

} // namespace bwm
