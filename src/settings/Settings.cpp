#include "settings/Settings.h"

#include <QCoreApplication>
#include <QSettings>

namespace bwm {

namespace {
constexpr int kDefaultPageWidth = 1080;
constexpr int kDefaultPageHeight = 1440;
constexpr int kDefaultAutoSaveIntervalMs = 5 * 60 * 1000;
}

QSettings &Settings::settings()
{
    // 组织名与应用名在 main.cpp 中设置（QCoreApplication::setOrganizationName 等）
    static QSettings instance;
    return instance;
}

QSize Settings::defaultPageSize()
{
    const QSize size = settings().value(QStringLiteral("defaultPageSize")).toSize();
    if (!size.isValid() || size.width() <= 0 || size.height() <= 0)
        return QSize(kDefaultPageWidth, kDefaultPageHeight);
    return size;
}

void Settings::setDefaultPageSize(const QSize &size)
{
    settings().setValue(QStringLiteral("defaultPageSize"), size);
}

int Settings::autoSaveIntervalMs()
{
    const int interval = settings().value(QStringLiteral("autoSaveIntervalMs"), kDefaultAutoSaveIntervalMs).toInt();
    return qMax(1000, interval);
}

void Settings::setAutoSaveIntervalMs(int intervalMs)
{
    settings().setValue(QStringLiteral("autoSaveIntervalMs"), qMax(1000, intervalMs));
}

QString Settings::authorName()
{
    return settings().value(QStringLiteral("authorName")).toString();
}

void Settings::setAuthorName(const QString &name)
{
    settings().setValue(QStringLiteral("authorName"), name);
}

QStringList Settings::recentProjects()
{
    return settings().value(QStringLiteral("recentProjects")).toStringList();
}

void Settings::setRecentProjects(const QStringList &paths)
{
    settings().setValue(QStringLiteral("recentProjects"), paths);
}

} // namespace bwm
