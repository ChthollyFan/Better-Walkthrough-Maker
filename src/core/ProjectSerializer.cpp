/**
 * @file ProjectSerializer.cpp
 * @author zhangweimu
 * @brief project.json 的序列化实现（读写与容错解析）。
 */
#include "core/ProjectSerializer.h"

#include "core/Component.h"
#include "core/ComponentSerializer.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace bwm {

namespace {

// 单页边长上限（逻辑像素）。导出上限见规划第 9 节（约 32767px），
// 此处设 16384 作为解析防线，防止损坏数据撑爆内存。
constexpr int nMaxPageDimension = 16384;
// 默认页面尺寸：与全局设置中的默认画布尺寸保持一致。
const QSize kDefaultPageSize(1080, 1440);

// 解析页面尺寸；数值缺失、非正数或超上限时回退默认值。
QSize parsePageSize(const QJsonObject& rPageObject)
{
    const int nWidth = rPageObject.value(QStringLiteral("width")).toInt(0);
    const int nHeight = rPageObject.value(QStringLiteral("height")).toInt(0);
    if (nWidth <= 0 || nHeight <= 0 || nWidth > nMaxPageDimension || nHeight > nMaxPageDimension) {
        return kDefaultPageSize;
    }
    return QSize(nWidth, nHeight);
}

QJsonObject walkthroughToJson(const Walkthrough& rWalkthrough)
{
    QJsonObject walkthroughObject;
    walkthroughObject.insert(QStringLiteral("title"), rWalkthrough.strTitle);
    walkthroughObject.insert(QStringLiteral("type"), walkthroughTypeToString(rWalkthrough.eType));

    QJsonArray pagesArray;
    for (const Page& rPage : rWalkthrough.vecPages) {
        QJsonObject pageObject;
        pageObject.insert(QStringLiteral("name"), rPage.strName);
        pageObject.insert(QStringLiteral("width"), rPage.size.width());
        pageObject.insert(QStringLiteral("height"), rPage.size.height());
        QJsonArray componentsArray;
        for (const Component& rComponent : rPage.vecComponents) {
            componentsArray.append(ComponentSerializer::toJson(rComponent));
        }
        pageObject.insert(QStringLiteral("components"), componentsArray);
        pagesArray.append(pageObject);
    }
    walkthroughObject.insert(QStringLiteral("pages"), pagesArray);
    return walkthroughObject;
}

Walkthrough walkthroughFromJson(const QJsonObject& rWalkthroughObject)
{
    Walkthrough walkthrough;
    walkthrough.strTitle = rWalkthroughObject.value(QStringLiteral("title")).toString(QStringLiteral("未命名攻略"));

    const QJsonValue typeValue = rWalkthroughObject.value(QStringLiteral("type"));
    walkthrough.eType = typeValue.isString()
        ? walkthroughTypeFromString(typeValue.toString())
        : E_WALKTHROUGH_TYPE_COVER;

    const QJsonValue pagesValue = rWalkthroughObject.value(QStringLiteral("pages"));
    if (pagesValue.isArray()) {
        const QJsonArray pagesArray = pagesValue.toArray();
        for (const QJsonValue& pageValue : pagesArray) {
            if (!pageValue.isObject()) {
                continue; // 跳过损坏条目，保证整份文件可打开
            }
            const QJsonObject pageObject = pageValue.toObject();
            Page page;
            page.strName = pageObject.value(QStringLiteral("name")).toString(QStringLiteral("未命名页面"));
            page.size = parsePageSize(pageObject);
            // 组件列表：旧文件无 components 字段时为空列表（向后兼容）
            const QJsonValue componentsValue = pageObject.value(QStringLiteral("components"));
            if (componentsValue.isArray()) {
                const QJsonArray componentsArray = componentsValue.toArray();
                for (const QJsonValue& rComponentValue : componentsArray) {
                    if (!rComponentValue.isObject()) {
                        continue;
                    }
                    page.vecComponents.append(ComponentSerializer::fromJson(rComponentValue.toObject()));
                }
            }
            walkthrough.vecPages.append(page);
        }
    }
    return walkthrough;
}

} // namespace

QString ProjectSerializer::toJson(const Project& rProject)
{
    QJsonObject root;
    root.insert(QStringLiteral("formatVersion"), nCurrentFormatVersion);
    root.insert(QStringLiteral("name"), rProject.strName);

    QJsonArray walkthroughsArray;
    for (const Walkthrough& rWalkthrough : rProject.vecWalkthroughs) {
        walkthroughsArray.append(walkthroughToJson(rWalkthrough));
    }
    root.insert(QStringLiteral("walkthroughs"), walkthroughsArray);

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool ProjectSerializer::fromJson(const QString& strJson, Project* pProject, QString* pErrorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(strJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("文件不是有效的 JSON：%1").arg(parseError.errorString());
        }
        return false;
    }

    const QJsonObject root = document.object();

    // 格式版本：缺失视为 1（手写文件友好）；高于当前版本拒绝打开
    const int nFormatVersion = root.value(QStringLiteral("formatVersion")).toInt(1);
    if (nFormatVersion < 1 || nFormatVersion > nCurrentFormatVersion) {
        if (pErrorMessage) {
            *pErrorMessage = nFormatVersion > nCurrentFormatVersion
                ? QStringLiteral("文件由更新版本的软件创建（格式版本 %1，当前支持 %2），请升级软件后打开")
                      .arg(nFormatVersion).arg(nCurrentFormatVersion)
                : QStringLiteral("格式版本 %1 不合法").arg(nFormatVersion);
        }
        return false;
    }

    Project parsed;
    parsed.strName = root.value(QStringLiteral("name")).toString();

    const QJsonValue walkthroughsValue = root.value(QStringLiteral("walkthroughs"));
    if (walkthroughsValue.isArray()) {
        const QJsonArray walkthroughsArray = walkthroughsValue.toArray();
        for (const QJsonValue& rValue : walkthroughsArray) {
            if (!rValue.isObject()) {
                continue;
            }
            parsed.vecWalkthroughs.append(walkthroughFromJson(rValue.toObject()));
        }
    }

    *pProject = parsed;
    return true;
}

} // namespace bwm
