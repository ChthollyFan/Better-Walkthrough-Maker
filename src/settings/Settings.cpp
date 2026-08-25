/**
 * @file Settings.cpp
 * @author zhangweimu
 * @brief 全局设置实现（QSettings 读写）。
 */
#include "settings/Settings.h"

#include <QCoreApplication>
#include <QSettings>

namespace bwm {

namespace {

constexpr int nDefaultPageWidth = 1080;
constexpr int nDefaultPageHeight = 1440;
constexpr int nDefaultAutoSaveIntervalMs = 5 * 60 * 1000;

} // namespace

QSettings& Settings::settings()
{
    // 组织名与应用名在 main.cpp 中设置（QCoreApplication::setOrganizationName 等）
    static QSettings instance;
    return instance;
}

QSize Settings::defaultPageSize()
{
    const QSize size = settings().value(QStringLiteral("defaultPageSize")).toSize();
    if (!size.isValid() || size.width() <= 0 || size.height() <= 0) {
        return QSize(nDefaultPageWidth, nDefaultPageHeight);
    }
    return size;
}

void Settings::setDefaultPageSize(const QSize& size)
{
    settings().setValue(QStringLiteral("defaultPageSize"), size);
}

int Settings::autoSaveIntervalMs()
{
    const int nInterval = settings().value(QStringLiteral("autoSaveIntervalMs"), nDefaultAutoSaveIntervalMs).toInt();
    return qMax(1000, nInterval);
}

void Settings::setAutoSaveIntervalMs(int nIntervalMs)
{
    settings().setValue(QStringLiteral("autoSaveIntervalMs"), qMax(1000, nIntervalMs));
}

QString Settings::authorName()
{
    return settings().value(QStringLiteral("authorName")).toString();
}

void Settings::setAuthorName(const QString& strName)
{
    settings().setValue(QStringLiteral("authorName"), strName);
}

QStringList Settings::recentProjects()
{
    return settings().value(QStringLiteral("recentProjects")).toStringList();
}

void Settings::setRecentProjects(const QStringList& vecPaths)
{
    settings().setValue(QStringLiteral("recentProjects"), vecPaths);
}

} // namespace bwm
