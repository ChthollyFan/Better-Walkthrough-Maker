/**
 * @file ComponentSerializer.cpp
 * @author zhangweimu
 * @brief 组件与 JSON 相互转换实现。
 */
#include "core/ComponentSerializer.h"

#include <QJsonArray>
#include <QJsonObject>

namespace bwm {

QJsonObject ComponentSerializer::toJson(const Component& rComponent)
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
    } else if (rComponent.eType == E_COMPONENT_TYPE_STICKER) {
        QJsonObject stickerObject;
        stickerObject.insert(QStringLiteral("stickerType"),
                             stickerTypeToString(rComponent.stickerData.eStickerType));
        stickerObject.insert(QStringLiteral("color"), colorToString(rComponent.stickerData.color));
        componentObject.insert(QStringLiteral("sticker"), stickerObject);
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

Component ComponentSerializer::fromJson(const QJsonObject& rComponentObject)
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

    const QJsonObject stickerObject = rComponentObject.value(QStringLiteral("sticker")).toObject();
    const QJsonValue stickerTypeValue = stickerObject.value(QStringLiteral("stickerType"));
    component.stickerData.eStickerType = stickerTypeValue.isString()
        ? stickerTypeFromString(stickerTypeValue.toString())
        : E_STICKER_TYPE_TITLE_LINE;
    component.stickerData.color = colorFromString(stickerObject.value(QStringLiteral("color")).toString());

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

} // namespace bwm
