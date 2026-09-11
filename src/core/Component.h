/**
 * @file Component.h
 * @author zhangweimu
 * @brief 画布组件数据模型（Model 与 GraphicsItem 分离，M2 画布阶段）。
 */
#ifndef BWM_CORE_COMPONENT_H
#define BWM_CORE_COMPONENT_H

#include <QColor>
#include <QPointF>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QVector>

namespace bwm {

// 组件类型
enum E_COMPONENT_TYPE {
    E_COMPONENT_TYPE_IMAGE = 0,   // 图片
    E_COMPONENT_TYPE_TEXT,        // 文本
    E_COMPONENT_TYPE_SHAPE,       // 形状
    E_COMPONENT_TYPE_TABLE,       // 表格
    E_COMPONENT_TYPE_STICKER,     // 贴纸（装饰素材）
};

// 贴纸子类型（内置装饰素材包第一版）
enum E_STICKER_TYPE {
    E_STICKER_TYPE_TITLE_LINE = 0,   // 标题装饰线
    E_STICKER_TYPE_CORNER_BADGE,     // 角标
    E_STICKER_TYPE_STAR_RATING,      // 推荐度星标
    E_STICKER_TYPE_ARROW,            // 箭头
    E_STICKER_TYPE_DIVIDER,          // 分割线
    E_STICKER_TYPE_CARD_BORDER,      // 卡片边框
};

// 卡片边框形状（仅 E_STICKER_TYPE_CARD_BORDER 使用）。
// 正方形/圆形取组件矩形的内接图形（居中）；椭圆/矩形填满组件矩形。
enum E_CARD_BORDER_SHAPE {
    E_CARD_BORDER_SHAPE_RECTANGLE = 0,   // 圆角矩形（默认，与旧版本一致）
    E_CARD_BORDER_SHAPE_SQUARE,          // 正方形（内接）
    E_CARD_BORDER_SHAPE_CIRCLE,          // 圆形（内接）
    E_CARD_BORDER_SHAPE_ELLIPSE,         // 椭圆（填满）
};

// 形状子类型
enum E_SHAPE_TYPE {
    E_SHAPE_TYPE_RECTANGLE = 0,   // 矩形
    E_SHAPE_TYPE_ROUND_RECT,      // 圆角矩形
    E_SHAPE_TYPE_ELLIPSE,         // 椭圆
    E_SHAPE_TYPE_LINE,            // 线条
};

// 图片组件数据：M3 素材库前先用文件路径，素材库上线后改为素材 id 引用。
struct ImageData {
    QString strAssetId;    // 素材 id（预留，M3 使用）
    QString strFilePath;   // 图片文件路径
};

// 文本组件数据：基础富文本样式（M2 范围）。
struct TextData {
    QString strContent;                    // 文本内容
    QString strFontFamily;                 // 字体
    int nFontSize = 24;                    // 字号
    QColor color = QColor(Qt::black);      // 文字颜色（仅 RGB，透明度见 nOpacityPercent）
    bool bBold = false;                    // 加粗
    int nAlign = Qt::AlignLeft;            // 对齐方式（Qt::Alignment 的 int 形式，便于序列化）
    // 不透明度（0~100）；100 = 完全不透明。颜色只存 #RRGGBB，
    // 透明度单列存储（colorToString 不保留 alpha），绘制时合成到颜色 alpha。
    int nOpacityPercent = 100;
};

// 形状组件数据
struct ShapeData {
    E_SHAPE_TYPE eShapeType = E_SHAPE_TYPE_RECTANGLE;   // 形状类型
    QColor fillColor = QColor(230, 230, 230);           // 填充色
    QColor borderColor = QColor(Qt::gray);              // 描边色
    int nBorderWidth = 1;                               // 描边宽度
};

// 表格组件数据：装备数值表、属性对比等。
struct TableData {
    QVector<QStringList> vecRows;        // 行数据（每行一个字符串列表）；第一行可作表头
    QColor headerColor = QColor(200, 200, 200);   // 表头背景色
    QColor textColor = QColor(Qt::black);        // 文本颜色
    QColor borderColor = QColor(Qt::gray);       // 边框颜色
    int nFontSize = 16;                           // 字号
    bool bShowHeader = true;                      // 是否显示表头
    bool bAlternateRow = false;                   // 斑马纹
};

// 贴纸组件数据：程序绘制的装饰元素（素材包第一版全部内置）。
struct StickerData {
    E_STICKER_TYPE eStickerType = E_STICKER_TYPE_TITLE_LINE;   // 贴纸类型
    QColor color = QColor(0, 120, 215);                        // 主色
    // 卡片边框形状（eStickerType == E_STICKER_TYPE_CARD_BORDER 时有效）。
    // 四种形状合并到「卡片边框」一个插入项，作为插入后（或插入时）可切换的选项，
    // 不在「插入」菜单里拆成多个菜单项。
    E_CARD_BORDER_SHAPE eBorderShape = E_CARD_BORDER_SHAPE_RECTANGLE;
    // 卡片边框内要展示的图片（项目内**相对路径**，如 "assets/xxx.png"；空 = 无图片）。
    // 绘制时按边框形状裁剪：框内显示、框外隐藏；用相对路径保证项目可整体移动。
    QString strImagePath;
    // 图片取景位置（0 = 贴左/上边缘，0.5 = 居中，1 = 贴右/下边缘）。
    // 图片按「等比覆盖」铺满边框区域，比例不符时由这两个值决定露出哪一部分。
    qreal dImageOffsetX = 0.5;
    qreal dImageOffsetY = 0.5;
};

// 组件：画布元素。数据与渲染分离——本结构仅存数据，渲染由 editor/ComponentItem 完成。
struct Component {
    QString strId;                        // 唯一 id（QUuid 字符串）
    E_COMPONENT_TYPE eType = E_COMPONENT_TYPE_SHAPE;
    QPointF pos;                          // 位置（页面左上角为原点）
    QSizeF size = QSizeF(200, 120);       // 尺寸
    qreal dRotation = 0;                  // 旋转角（度）
    int nZOrder = 0;                      // 图层顺序（数值大者在上层）
    bool bVisible = true;                 // 可见
    bool bLocked = false;                 // 锁定（不可移动/缩放）

    ImageData imageData;                  // 图片数据（eType 为 IMAGE 时有效）
    TextData textData;                    // 文本数据（eType 为 TEXT 时有效）
    ShapeData shapeData;                  // 形状数据（eType 为 SHAPE 时有效）
    TableData tableData;                  // 表格数据（eType 为 TABLE 时有效）
    StickerData stickerData;              // 贴纸数据（eType 为 STICKER 时有效）
};

// 组件类型与颜色的字符串转换（JSON 序列化用；字符串形式保证可读与迁移友好）
QString componentTypeToString(E_COMPONENT_TYPE eType);
E_COMPONENT_TYPE componentTypeFromString(const QString& strType);
QString shapeTypeToString(E_SHAPE_TYPE eShapeType);
E_SHAPE_TYPE shapeTypeFromString(const QString& strShapeType);
QString stickerTypeToString(E_STICKER_TYPE eStickerType);
E_STICKER_TYPE stickerTypeFromString(const QString& strStickerType);
QString cardBorderShapeToString(E_CARD_BORDER_SHAPE eShape);
E_CARD_BORDER_SHAPE cardBorderShapeFromString(const QString& strShape);
QString colorToString(const QColor& rColor);
QColor colorFromString(const QString& strColor);
// 文本组件默认字体族（TextData::strFontFamily 为空时使用）；绘制与设置界面共用同一来源
QString textDefaultFontFamily();

// 相等比较（快照撤销、脏检测等场景使用）
inline bool operator==(const ImageData& rLeft, const ImageData& rRight)
{
    return rLeft.strAssetId == rRight.strAssetId && rLeft.strFilePath == rRight.strFilePath;
}

inline bool operator==(const TextData& rLeft, const TextData& rRight)
{
    return rLeft.strContent == rRight.strContent
        && rLeft.strFontFamily == rRight.strFontFamily
        && rLeft.nFontSize == rRight.nFontSize
        && rLeft.color == rRight.color
        && rLeft.bBold == rRight.bBold
        && rLeft.nAlign == rRight.nAlign
        && rLeft.nOpacityPercent == rRight.nOpacityPercent;
}

inline bool operator==(const ShapeData& rLeft, const ShapeData& rRight)
{
    return rLeft.eShapeType == rRight.eShapeType
        && rLeft.fillColor == rRight.fillColor
        && rLeft.borderColor == rRight.borderColor
        && rLeft.nBorderWidth == rRight.nBorderWidth;
}

inline bool operator==(const TableData& rLeft, const TableData& rRight)
{
    return rLeft.vecRows == rRight.vecRows
        && rLeft.headerColor == rRight.headerColor
        && rLeft.textColor == rRight.textColor
        && rLeft.borderColor == rRight.borderColor
        && rLeft.nFontSize == rRight.nFontSize
        && rLeft.bShowHeader == rRight.bShowHeader
        && rLeft.bAlternateRow == rRight.bAlternateRow;
}

inline bool operator==(const StickerData& rLeft, const StickerData& rRight)
{
    return rLeft.eStickerType == rRight.eStickerType
        && rLeft.color == rRight.color
        && rLeft.eBorderShape == rRight.eBorderShape
        && rLeft.strImagePath == rRight.strImagePath
        && qFuzzyCompare(rLeft.dImageOffsetX, rRight.dImageOffsetX)
        && qFuzzyCompare(rLeft.dImageOffsetY, rRight.dImageOffsetY);
}

inline bool operator==(const Component& rLeft, const Component& rRight)
{
    return rLeft.strId == rRight.strId
        && rLeft.eType == rRight.eType
        && rLeft.pos == rRight.pos
        && rLeft.size == rRight.size
        && rLeft.dRotation == rRight.dRotation
        && rLeft.nZOrder == rRight.nZOrder
        && rLeft.bVisible == rRight.bVisible
        && rLeft.bLocked == rRight.bLocked
        && rLeft.imageData == rRight.imageData
        && rLeft.textData == rRight.textData
        && rLeft.shapeData == rRight.shapeData
        && rLeft.tableData == rRight.tableData
        && rLeft.stickerData == rRight.stickerData;
}

inline bool operator!=(const Component& rLeft, const Component& rRight)
{
    return !(rLeft == rRight);
}

} // namespace bwm

#endif // BWM_CORE_COMPONENT_H
