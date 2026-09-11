/**
 * @file Settings.cpp
 * @author zhangweimu
 * @brief 全局设置实现（QSettings 读写）。
 */
#include "settings/Settings.h"

#include "core/Component.h"   // colorToString / colorFromString（署名颜色持久化复用）

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

AuthorMarkStyle Settings::authorMarkStyle()
{
    // 逐项读取：任一键缺失时退回 AuthorMarkStyle 的默认值（等价旧版硬编码水印）
    AuthorMarkStyle style;
    style.ePosition = authorMarkPositionFromString(
        settings().value(QStringLiteral("authorMark/position")).toString());
    style.strFontFamily = settings().value(QStringLiteral("authorMark/fontFamily")).toString();
    style.nFontSize = settings().value(QStringLiteral("authorMark/fontSize"),
                                       AuthorMarkStyle::nDefaultFontSize).toInt();
    style.bBold = settings().value(QStringLiteral("authorMark/bold"), false).toBool();
    // 颜色只存 #RRGGBB（colorToString 不保留 alpha，透明度由 nOpacityPercent 单独负责）
    style.color = colorFromString(
        settings().value(QStringLiteral("authorMark/color")).toString());
    style.nOpacityPercent = settings().value(QStringLiteral("authorMark/opacity"),
                                             AuthorMarkStyle::nDefaultOpacityPercent).toInt();
    style.clamp();   // 配置被手改出越界值时收敛，避免绘制异常
    return style;
}

void Settings::setAuthorMarkStyle(const AuthorMarkStyle& rStyle)
{
    AuthorMarkStyle style = rStyle;
    style.clamp();
    settings().setValue(QStringLiteral("authorMark/position"),
                        authorMarkPositionToString(style.ePosition));
    settings().setValue(QStringLiteral("authorMark/fontFamily"), style.strFontFamily);
    settings().setValue(QStringLiteral("authorMark/fontSize"), style.nFontSize);
    settings().setValue(QStringLiteral("authorMark/bold"), style.bBold);
    settings().setValue(QStringLiteral("authorMark/color"), colorToString(style.color));
    settings().setValue(QStringLiteral("authorMark/opacity"), style.nOpacityPercent);
}

QStringList Settings::recentProjects()
{
    return settings().value(QStringLiteral("recentProjects")).toStringList();
}

void Settings::setRecentProjects(const QStringList& vecPaths)
{
    settings().setValue(QStringLiteral("recentProjects"), vecPaths);
}

QString Settings::themeName()
{
    return settings().value(QStringLiteral("theme"), QStringLiteral("浅色页面")).toString();
}

void Settings::setThemeName(const QString& rName)
{
    settings().setValue(QStringLiteral("theme"), rName);
}

QString Settings::uiStyle()
{
    return settings().value(QStringLiteral("ui/style"),
                            QStringLiteral("auto")).toString();
}

void Settings::setUiStyle(const QString& rId)
{
    settings().setValue(QStringLiteral("ui/style"), rId);
}

QByteArray Settings::windowGeometry()
{
    return settings().value(QStringLiteral("windowGeometry")).toByteArray();
}

void Settings::setWindowGeometry(const QByteArray& rGeometry)
{
    if (!rGeometry.isEmpty()) {
        settings().setValue(QStringLiteral("windowGeometry"), rGeometry);
    }
}

QString Settings::lastExportDirectory()
{
    return settings().value(QStringLiteral("lastExportDirectory")).toString();
}

void Settings::setLastExportDirectory(const QString& strDir)
{
    if (!strDir.trimmed().isEmpty()) {
        settings().setValue(QStringLiteral("lastExportDirectory"), strDir);
    }
}

} // namespace bwm
