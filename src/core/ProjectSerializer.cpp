/**
 * @file ProjectSerializer.cpp
 * @author zhangweimu
 * @brief project.json 的序列化实现（读写与容错解析）。
 */
#include "core/ProjectSerializer.h"

#include "core/Component.h"

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

// 组件 → JSON 对象
QJsonObject componentToJson(const Component& rComponent)
{
    QJsonObject componentObject;
    componentObject.insert(QStringLiteral("id"), rComponent.strId);
    componentObject.insert(QStringLiteral("type"), componentTypeToString(rComponent.eType));
    componentObject.insert(QStringLiteral("posX"), rComponent.pos.x());
    componentObject.insert(QStringLiteral("posY"), rComponent.pos.y());
    componentObject.insert(QStringLiteral("width"), rComponent.size.width());
    componentObject.insert(QStringLiteral("height"), rComponent.size.height());
    componentObject.insert(QStringLiteral("rotation"), rComponent.dRotation);
    componentObject.insert(QStringLiteral("zOrder"), rComponent.nZOrder);
    componentObject.insert(QStringLiteral("visible"), rComponent.bVisible);
    componentObject.insert(QStringLiteral("locked"), rComponent.bLocked);

    if (rComponent.eType == E_COMPONENT_TYPE_IMAGE) {
        QJsonObject imageObject;
        imageObject.insert(QStringLiteral("assetId"), rComponent.imageData.strAssetId);
        imageObject.insert(QStringLiteral("filePath"), rComponent.imageData.strFilePath);
        componentObject.insert(QStringLiteral("image"), imageObject);
    } else if (rComponent.eType == E_COMPONENT_TYPE_TEXT) {
        QJsonObject textObject;
        textObject.insert(QStringLiteral("content"), rComponent.textData.strContent);
        textObject.insert(QStringLiteral("fontFamily"), rComponent.textData.strFontFamily);
        textObject.insert(QStringLiteral("fontSize"), rComponent.textData.nFontSize);
        textObject.insert(QStringLiteral("color"), colorToString(rComponent.textData.color));
        textObject.insert(QStringLiteral("bold"), rComponent.textData.bBold);
        textObject.insert(QStringLiteral("align"), rComponent.textData.nAlign);
        componentObject.insert(QStringLiteral("text"), textObject);
    } else if (rComponent.eType == E_COMPONENT_TYPE_TABLE) {
        QJsonObject tableObject;
        QJsonArray rowsArray;
        for (const QStringList& rRow : rComponent.tableData.vecRows) {
            QJsonArray rowArray;
            for (const QString& rCell : rRow) {
                rowArray.append(rCell);
            }
            rowsArray.append(rowArray);
        }
        tableObject.insert(QStringLiteral("rows"), rowsArray);
        tableObject.insert(QStringLiteral("headerColor"), colorToString(rComponent.tableData.headerColor));
        tableObject.insert(QStringLiteral("textColor"), colorToString(rComponent.tableData.textColor));
        tableObject.insert(QStringLiteral("borderColor"), colorToString(rComponent.tableData.borderColor));
        tableObject.insert(QStringLiteral("fontSize"), rComponent.tableData.nFontSize);
        tableObject.insert(QStringLiteral("showHeader"), rComponent.tableData.bShowHeader);
        tableObject.insert(QStringLiteral("alternateRow"), rComponent.tableData.bAlternateRow);
        componentObject.insert(QStringLiteral("table"), tableObject);
    } else {
        QJsonObject shapeObject;
        shapeObject.insert(QStringLiteral("shapeType"), shapeTypeToString(rComponent.shapeData.eShapeType));
        shapeObject.insert(QStringLiteral("fillColor"), colorToString(rComponent.shapeData.fillColor));
        shapeObject.insert(QStringLiteral("borderColor"), colorToString(rComponent.shapeData.borderColor));
        shapeObject.insert(QStringLiteral("borderWidth"), rComponent.shapeData.nBorderWidth);
        componentObject.insert(QStringLiteral("shape"), shapeObject);
    }
    return componentObject;
}

// JSON 对象 → 组件；缺失字段取默认值
Component componentFromJson(const QJsonObject& rComponentObject)
{
    Component component;
    component.strId = rComponentObject.value(QStringLiteral("id")).toString();
    const QJsonValue typeValue = rComponentObject.value(QStringLiteral("type"));
    component.eType = typeValue.isString()
        ? componentTypeFromString(typeValue.toString())
        : E_COMPONENT_TYPE_SHAPE;
    component.pos = QPointF(rComponentObject.value(QStringLiteral("posX")).toDouble(0),
                            rComponentObject.value(QStringLiteral("posY")).toDouble(0));
    component.size = QSizeF(rComponentObject.value(QStringLiteral("width")).toDouble(200),
                            rComponentObject.value(QStringLiteral("height")).toDouble(120));
    component.dRotation = rComponentObject.value(QStringLiteral("rotation")).toDouble(0);
    component.nZOrder = rComponentObject.value(QStringLiteral("zOrder")).toInt(0);
    component.bVisible = rComponentObject.value(QStringLiteral("visible")).toBool(true);
    component.bLocked = rComponentObject.value(QStringLiteral("locked")).toBool(false);

    const QJsonObject imageObject = rComponentObject.value(QStringLiteral("image")).toObject();
    component.imageData.strAssetId = imageObject.value(QStringLiteral("assetId")).toString();
    component.imageData.strFilePath = imageObject.value(QStringLiteral("filePath")).toString();

    const QJsonObject textObject = rComponentObject.value(QStringLiteral("text")).toObject();
    component.textData.strContent = textObject.value(QStringLiteral("content")).toString();
    component.textData.strFontFamily = textObject.value(QStringLiteral("fontFamily")).toString();
    component.textData.nFontSize = textObject.value(QStringLiteral("fontSize")).toInt(24);
    component.textData.color = colorFromString(textObject.value(QStringLiteral("color")).toString());
    component.textData.bBold = textObject.value(QStringLiteral("bold")).toBool(false);
    component.textData.nAlign = textObject.value(QStringLiteral("align")).toInt(Qt::AlignLeft);

    const QJsonObject tableObject = rComponentObject.value(QStringLiteral("table")).toObject();
    component.tableData.vecRows.clear();
    const QJsonArray rowsArray = tableObject.value(QStringLiteral("rows")).toArray();
    for (const QJsonValue& rRowValue : rowsArray) {
        if (!rRowValue.isArray()) {
            continue;
        }
        QStringList row;
        const QJsonArray rowArray = rRowValue.toArray();
        for (const QJsonValue& rCellValue : rowArray) {
            row.append(rCellValue.toString());
        }
        component.tableData.vecRows.append(row);
    }
    component.tableData.headerColor = colorFromString(tableObject.value(QStringLiteral("headerColor")).toString());
    component.tableData.textColor = colorFromString(tableObject.value(QStringLiteral("textColor")).toString());
    component.tableData.borderColor = colorFromString(tableObject.value(QStringLiteral("borderColor")).toString());
    component.tableData.nFontSize = tableObject.value(QStringLiteral("fontSize")).toInt(16);
    component.tableData.bShowHeader = tableObject.value(QStringLiteral("showHeader")).toBool(true);
    component.tableData.bAlternateRow = tableObject.value(QStringLiteral("alternateRow")).toBool(false);

    const QJsonObject shapeObject = rComponentObject.value(QStringLiteral("shape")).toObject();
    const QJsonValue shapeTypeValue = shapeObject.value(QStringLiteral("shapeType"));
    component.shapeData.eShapeType = shapeTypeValue.isString()
        ? shapeTypeFromString(shapeTypeValue.toString())
        : E_SHAPE_TYPE_RECTANGLE;
    component.shapeData.fillColor = colorFromString(shapeObject.value(QStringLiteral("fillColor")).toString());
    component.shapeData.borderColor = colorFromString(shapeObject.value(QStringLiteral("borderColor")).toString());
    component.shapeData.nBorderWidth = shapeObject.value(QStringLiteral("borderWidth")).toInt(1);
    return component;
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
            componentsArray.append(componentToJson(rComponent));
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
                    page.vecComponents.append(componentFromJson(rComponentValue.toObject()));
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
