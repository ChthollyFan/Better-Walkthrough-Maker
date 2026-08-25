#include "core/ProjectSerializer.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace bwm {

namespace {

// 单页边长上限（逻辑像素）。导出上限见规划第 9 节（约 32767px），
// 此处设 16384 作为解析防线，防止损坏数据撑爆内存。
constexpr int kMaxPageDimension = 16384;
// 默认页面尺寸：与全局设置中的默认画布尺寸保持一致。
const QSize kDefaultPageSize(1080, 1440);

// 解析页面尺寸；数值缺失、非正数或超上限时回退默认值。
QSize parsePageSize(const QJsonObject &pageObject)
{
    const int width = pageObject.value(QStringLiteral("width")).toInt(0);
    const int height = pageObject.value(QStringLiteral("height")).toInt(0);
    if (width <= 0 || height <= 0 || width > kMaxPageDimension || height > kMaxPageDimension)
        return kDefaultPageSize;
    return QSize(width, height);
}

QJsonObject walkthroughToJson(const Walkthrough &walkthrough)
{
    QJsonObject walkthroughObject;
    walkthroughObject.insert(QStringLiteral("title"), walkthrough.title);
    walkthroughObject.insert(QStringLiteral("type"), walkthroughTypeToString(walkthrough.type));

    QJsonArray pagesArray;
    for (const Page &page : walkthrough.pages) {
        QJsonObject pageObject;
        pageObject.insert(QStringLiteral("name"), page.name);
        pageObject.insert(QStringLiteral("width"), page.size.width());
        pageObject.insert(QStringLiteral("height"), page.size.height());
        pagesArray.append(pageObject);
    }
    walkthroughObject.insert(QStringLiteral("pages"), pagesArray);
    return walkthroughObject;
}

Walkthrough walkthroughFromJson(const QJsonObject &walkthroughObject)
{
    Walkthrough walkthrough;
    walkthrough.title = walkthroughObject.value(QStringLiteral("title")).toString(QStringLiteral("未命名攻略"));

    const QJsonValue typeValue = walkthroughObject.value(QStringLiteral("type"));
    walkthrough.type = typeValue.isString()
        ? walkthroughTypeFromString(typeValue.toString())
        : WalkthroughType::Cover;

    const QJsonValue pagesValue = walkthroughObject.value(QStringLiteral("pages"));
    if (pagesValue.isArray()) {
        const QJsonArray pagesArray = pagesValue.toArray();
        for (const QJsonValue &pageValue : pagesArray) {
            if (!pageValue.isObject())
                continue; // 跳过损坏条目，保证整份文件可打开
            const QJsonObject pageObject = pageValue.toObject();
            Page page;
            page.name = pageObject.value(QStringLiteral("name")).toString(QStringLiteral("未命名页面"));
            page.size = parsePageSize(pageObject);
            walkthrough.pages.append(page);
        }
    }
    return walkthrough;
}

} // namespace

QString ProjectSerializer::toJson(const Project &project)
{
    QJsonObject root;
    root.insert(QStringLiteral("formatVersion"), kCurrentFormatVersion);
    root.insert(QStringLiteral("name"), project.name);

    QJsonArray walkthroughsArray;
    for (const Walkthrough &walkthrough : project.walkthroughs)
        walkthroughsArray.append(walkthroughToJson(walkthrough));
    root.insert(QStringLiteral("walkthroughs"), walkthroughsArray);

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool ProjectSerializer::fromJson(const QString &json, Project *project, QString *errorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("文件不是有效的 JSON：%1").arg(parseError.errorString());
        return false;
    }

    const QJsonObject root = document.object();

    // 格式版本：缺失视为 1（手写文件友好）；高于当前版本拒绝打开
    const int formatVersion = root.value(QStringLiteral("formatVersion")).toInt(1);
    if (formatVersion < 1 || formatVersion > kCurrentFormatVersion) {
        if (errorMessage) {
            *errorMessage = formatVersion > kCurrentFormatVersion
                ? QStringLiteral("文件由更新版本的软件创建（格式版本 %1，当前支持 %2），请升级软件后打开")
                      .arg(formatVersion).arg(kCurrentFormatVersion)
                : QStringLiteral("格式版本 %1 不合法").arg(formatVersion);
        }
        return false;
    }

    Project parsed;
    parsed.name = root.value(QStringLiteral("name")).toString();

    const QJsonValue walkthroughsValue = root.value(QStringLiteral("walkthroughs"));
    if (walkthroughsValue.isArray()) {
        const QJsonArray walkthroughsArray = walkthroughsValue.toArray();
        for (const QJsonValue &value : walkthroughsArray) {
            if (!value.isObject())
                continue;
            parsed.walkthroughs.append(walkthroughFromJson(value.toObject()));
        }
    }

    *project = parsed;
    return true;
}

} // namespace bwm
