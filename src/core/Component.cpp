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
    case E_COMPONENT_TYPE_TABLE:
        return QStringLiteral("table");
    case E_COMPONENT_TYPE_STICKER:
        return QStringLiteral("sticker");
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
    if (strType == QStringLiteral("table")) {
        return E_COMPONENT_TYPE_TABLE;
    }
    if (strType == QStringLiteral("sticker")) {
        return E_COMPONENT_TYPE_STICKER;
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

QString stickerTypeToString(E_STICKER_TYPE eStickerType)
{
    switch (eStickerType) {
    case E_STICKER_TYPE_TITLE_LINE:
        return QStringLiteral("title_line");
    case E_STICKER_TYPE_CORNER_BADGE:
        return QStringLiteral("corner_badge");
    case E_STICKER_TYPE_STAR_RATING:
        return QStringLiteral("star_rating");
    case E_STICKER_TYPE_ARROW:
        return QStringLiteral("arrow");
    case E_STICKER_TYPE_DIVIDER:
        return QStringLiteral("divider");
    case E_STICKER_TYPE_CARD_BORDER:
        return QStringLiteral("card_border");
    default:
        return QStringLiteral("title_line");
    }
}

E_STICKER_TYPE stickerTypeFromString(const QString& strStickerType)
{
    if (strStickerType == QStringLiteral("corner_badge")) {
        return E_STICKER_TYPE_CORNER_BADGE;
    }
    if (strStickerType == QStringLiteral("star_rating")) {
        return E_STICKER_TYPE_STAR_RATING;
    }
    if (strStickerType == QStringLiteral("arrow")) {
        return E_STICKER_TYPE_ARROW;
    }
    if (strStickerType == QStringLiteral("divider")) {
        return E_STICKER_TYPE_DIVIDER;
    }
    if (strStickerType == QStringLiteral("card_border")) {
        return E_STICKER_TYPE_CARD_BORDER;
    }
    return E_STICKER_TYPE_TITLE_LINE;
}

QString cardBorderShapeToString(E_CARD_BORDER_SHAPE eShape)
{
    switch (eShape) {
    case E_CARD_BORDER_SHAPE_SQUARE:
        return QStringLiteral("square");
    case E_CARD_BORDER_SHAPE_CIRCLE:
        return QStringLiteral("circle");
    case E_CARD_BORDER_SHAPE_ELLIPSE:
        return QStringLiteral("ellipse");
    case E_CARD_BORDER_SHAPE_RECTANGLE:
    default:
        return QStringLiteral("rectangle");
    }
}

E_CARD_BORDER_SHAPE cardBorderShapeFromString(const QString& strShape)
{
    if (strShape == QStringLiteral("square")) {
        return E_CARD_BORDER_SHAPE_SQUARE;
    }
    if (strShape == QStringLiteral("circle")) {
        return E_CARD_BORDER_SHAPE_CIRCLE;
    }
    if (strShape == QStringLiteral("ellipse")) {
        return E_CARD_BORDER_SHAPE_ELLIPSE;
    }
    // 缺失或未知形状按矩形处理，保证旧文件可打开
    return E_CARD_BORDER_SHAPE_RECTANGLE;
}

QString colorToString(const QColor& rColor)
{
    // 统一输出 #RRGGBB：颜色不存 alpha（兼容旧文件），
    // 需要透明度的组件（文本、署名水印）另用单独字段表示
    return rColor.name(QColor::HexRgb);
}

QColor colorFromString(const QString& strColor)
{
    const QColor color(strColor);
    return color.isValid() ? color : QColor(Qt::black);
}

QString textDefaultFontFamily()
{
    // 默认字体族：历史上硬编码在 ComponentPainter，现收敛到此处，
    // 供绘制与「文本样式」设置界面共用，避免两处默认值不一致
    return QStringLiteral("Microsoft YaHei");
}

} // namespace bwm
