/**
 * @file Template.cpp
 * @author zhangweimu
 * @brief 模板序列化实现。
 */
#include "core/Template.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "core/ComponentSerializer.h"

namespace bwm {

namespace {

constexpr int nMaxPageDimension = 16384;
const QSize kDefaultPageSize(1080, 1440);

QSize parsePageSize(const QJsonObject& rPageObject)
{
    const int nWidth = rPageObject.value(QStringLiteral("width")).toInt(0);
    const int nHeight = rPageObject.value(QStringLiteral("height")).toInt(0);
    if (nWidth <= 0 || nHeight <= 0 || nWidth > nMaxPageDimension || nHeight > nMaxPageDimension) {
        return kDefaultPageSize;
    }
    return QSize(nWidth, nHeight);
}

QJsonObject pageToJson(const Page& rPage)
{
    QJsonObject pageObject;
    pageObject.insert(QStringLiteral("name"), rPage.strName);
    pageObject.insert(QStringLiteral("width"), rPage.size.width());
    pageObject.insert(QStringLiteral("height"), rPage.size.height());
    QJsonArray componentsArray;
    for (const Component& rComponent : rPage.vecComponents) {
        componentsArray.append(ComponentSerializer::toJson(rComponent));
    }
    pageObject.insert(QStringLiteral("components"), componentsArray);
    return pageObject;
}

Page pageFromJson(const QJsonObject& rPageObject)
{
    Page page;
    page.strName = rPageObject.value(QStringLiteral("name")).toString(QStringLiteral("页面"));
    page.size = parsePageSize(rPageObject);
    const QJsonValue componentsValue = rPageObject.value(QStringLiteral("components"));
    if (componentsValue.isArray()) {
        const QJsonArray componentsArray = componentsValue.toArray();
        for (const QJsonValue& rComponentValue : componentsArray) {
            if (!rComponentValue.isObject()) {
                continue;
            }
            page.vecComponents.append(ComponentSerializer::fromJson(rComponentValue.toObject()));
        }
    }
    return page;
}

} // namespace

QString TemplateSerializer::toJson(const Template& rTemplate)
{
    QJsonObject root;
    root.insert(QStringLiteral("formatVersion"), nCurrentFormatVersion);
    root.insert(QStringLiteral("name"), rTemplate.strName);
    root.insert(QStringLiteral("type"), walkthroughTypeToString(rTemplate.eType));
    root.insert(QStringLiteral("description"), rTemplate.strDescription);

    QJsonArray pagesArray;
    for (const Page& rPage : rTemplate.vecPages) {
        pagesArray.append(pageToJson(rPage));
    }
    root.insert(QStringLiteral("pages"), pagesArray);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool TemplateSerializer::fromJson(const QString& rJson, Template* pTemplate, QString* pErrorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(rJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("文件不是有效的 JSON：%1").arg(parseError.errorString());
        }
        return false;
    }
    const QJsonObject root = document.object();

    const int nFormatVersion = root.value(QStringLiteral("formatVersion")).toInt(1);
    if (nFormatVersion < 1 || nFormatVersion > nCurrentFormatVersion) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("模板格式版本 %1 不受支持（当前支持 %2）")
                                 .arg(nFormatVersion).arg(nCurrentFormatVersion);
        }
        return false;
    }

    Template parsed;
    parsed.strName = root.value(QStringLiteral("name")).toString();
    const QJsonValue typeValue = root.value(QStringLiteral("type"));
    parsed.eType = typeValue.isString()
        ? walkthroughTypeFromString(typeValue.toString())
        : E_WALKTHROUGH_TYPE_COVER;
    parsed.strDescription = root.value(QStringLiteral("description")).toString();

    const QJsonValue pagesValue = root.value(QStringLiteral("pages"));
    if (pagesValue.isArray()) {
        const QJsonArray pagesArray = pagesValue.toArray();
        for (const QJsonValue& rPageValue : pagesArray) {
            if (!rPageValue.isObject()) {
                continue;
            }
            parsed.vecPages.append(pageFromJson(rPageValue.toObject()));
        }
    }
    if (parsed.vecPages.isEmpty()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("模板不包含任何页面");
        }
        return false;
    }

    *pTemplate = parsed;
    return true;
}

bool TemplateSerializer::writeFile(const Template& rTemplate, const QString& strFilePath,
                                   QString* pErrorMessage)
{
    QSaveFile file(strFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("无法写入模板文件：%1").arg(strFilePath);
        }
        return false;
    }
    file.write(toJson(rTemplate).toUtf8());
    if (!file.commit()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("写入模板文件失败：%1").arg(strFilePath);
        }
        return false;
    }
    return true;
}

bool TemplateSerializer::readFile(const QString& strFilePath, Template* pTemplate,
                                  QString* pErrorMessage)
{
    QFile file(strFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("无法读取模板文件：%1").arg(strFilePath);
        }
        return false;
    }
    const QString strJson = QString::fromUtf8(file.readAll());
    file.close();
    return fromJson(strJson, pTemplate, pErrorMessage);
}

} // namespace bwm
