/**
 * @file Component.cpp
 * @author zhangweimu
 * @brief 组件数据模型的辅助函数（类型与颜色字符串转换，用于 JSON 序列化）。
 */
#include "core/Component.h"

namespace bwm {

QString componentTypeToString(E_COMPONENT_TYPE eType)
{
    switch (eType) {
    case E_COMPONENT_TYPE_IMAGE:
        return QStringLiteral("image");
    case E_COMPONENT_TYPE_TEXT:
        return QStringLiteral("text");
    case E_COMPONENT_TYPE_SHAPE:
        return QStringLiteral("shape");
    default:
        return QStringLiteral("shape");
    }
}

E_COMPONENT_TYPE componentTypeFromString(const QString& strType)
{
    if (strType == QStringLiteral("image")) {
        return E_COMPONENT_TYPE_IMAGE;
    }
    if (strType == QStringLiteral("text")) {
        return E_COMPONENT_TYPE_TEXT;
    }
    // 未知类型按形状处理，保证文件可打开
    return E_COMPONENT_TYPE_SHAPE;
}

QString shapeTypeToString(E_SHAPE_TYPE eShapeType)
{
    switch (eShapeType) {
    case E_SHAPE_TYPE_RECTANGLE:
        return QStringLiteral("rectangle");
    case E_SHAPE_TYPE_ROUND_RECT:
        return QStringLiteral("round_rect");
    case E_SHAPE_TYPE_ELLIPSE:
        return QStringLiteral("ellipse");
    case E_SHAPE_TYPE_LINE:
        return QStringLiteral("line");
    default:
        return QStringLiteral("rectangle");
    }
}

E_SHAPE_TYPE shapeTypeFromString(const QString& strShapeType)
{
    if (strShapeType == QStringLiteral("round_rect")) {
        return E_SHAPE_TYPE_ROUND_RECT;
    }
    if (strShapeType == QStringLiteral("ellipse")) {
        return E_SHAPE_TYPE_ELLIPSE;
    }
    if (strShapeType == QStringLiteral("line")) {
        return E_SHAPE_TYPE_LINE;
    }
    return E_SHAPE_TYPE_RECTANGLE;
}

QString colorToString(const QColor& rColor)
{
    // 统一输出 #RRGGBB（不支持透明时省略 alpha；M2 范围无透明度需求）
    return rColor.name(QColor::HexRgb);
}

QColor colorFromString(const QString& strColor)
{
    const QColor color(strColor);
    return color.isValid() ? color : QColor(Qt::black);
}

} // namespace bwm
