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
    QColor color = QColor(Qt::black);      // 文字颜色
    bool bBold = false;                    // 加粗
    int nAlign = Qt::AlignLeft;            // 对齐方式（Qt::Alignment 的 int 形式，便于序列化）
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
};

// 组件类型与颜色的字符串转换（JSON 序列化用；字符串形式保证可读与迁移友好）
QString componentTypeToString(E_COMPONENT_TYPE eType);
E_COMPONENT_TYPE componentTypeFromString(const QString& strType);
QString shapeTypeToString(E_SHAPE_TYPE eShapeType);
E_SHAPE_TYPE shapeTypeFromString(const QString& strShapeType);
QString colorToString(const QColor& rColor);
QColor colorFromString(const QString& strColor);

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
        && rLeft.nAlign == rRight.nAlign;
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
        && rLeft.tableData == rRight.tableData;
}

inline bool operator!=(const Component& rLeft, const Component& rRight)
{
    return !(rLeft == rRight);
}

} // namespace bwm

#endif // BWM_CORE_COMPONENT_H
