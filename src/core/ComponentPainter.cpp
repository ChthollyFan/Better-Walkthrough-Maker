/**
 * @file ComponentPainter.cpp
 * @author zhangweimu
 * @brief 组件统一绘制实现（图片 / 文本 / 表格 / 形状）。
 */
#include "core/ComponentPainter.h"

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QtMath>

namespace bwm {

namespace {

// 绘制五角星（空心），用于推荐度星标
void drawStar(QPainter* pPainter, const QPointF& rCenter, qreal dRadius)
{
    QPolygonF polygon;
    for (int nIndex = 0; nIndex < 10; ++nIndex) {
        const qreal dAngle = -M_PI / 2 + nIndex * M_PI / 5;
        const qreal dR = (nIndex % 2 == 0) ? dRadius : dRadius * 0.4;
        polygon << QPointF(rCenter.x() + dR * std::cos(dAngle),
                           rCenter.y() + dR * std::sin(dAngle));
    }
    pPainter->drawPolygon(polygon);
}

} // namespace

void ComponentPainter::paint(QPainter* pPainter, const Component& rComponent,
                             const QRectF& rContentRect, QImage* pImageCache)
{
    if (rComponent.eType == E_COMPONENT_TYPE_IMAGE) {
        QImage image;
        if (pImageCache) {
            if (pImageCache->isNull() && !rComponent.imageData.strFilePath.isEmpty()) {
                pImageCache->load(rComponent.imageData.strFilePath);
            }
            image = *pImageCache;
        } else if (!rComponent.imageData.strFilePath.isEmpty()) {
            image.load(rComponent.imageData.strFilePath);
        }

        if (!image.isNull()) {
            // 保持纵横比居中绘制
            const QSizeF scaled = image.size().scaled(rContentRect.size().toSize(), Qt::KeepAspectRatio);
            const QRectF targetRect(rContentRect.x() + (rContentRect.width() - scaled.width()) / 2,
                                    rContentRect.y() + (rContentRect.height() - scaled.height()) / 2,
                                    scaled.width(), scaled.height());
            pPainter->drawImage(targetRect, image);
        } else {
            pPainter->fillRect(rContentRect, QColor(220, 220, 220));
            pPainter->drawText(rContentRect, Qt::AlignCenter, QStringLiteral("图片加载失败"));
        }
        return;
    }

    if (rComponent.eType == E_COMPONENT_TYPE_TEXT) {
        const TextData& rText = rComponent.textData;
        QFont font(rText.strFontFamily.isEmpty() ? QStringLiteral("Microsoft YaHei") : rText.strFontFamily);
        font.setPixelSize(rText.nFontSize);
        font.setBold(rText.bBold);
        pPainter->setFont(font);
        pPainter->setPen(rText.color);
        pPainter->drawText(rContentRect, rText.nAlign, rText.strContent);
        return;
    }

    if (rComponent.eType == E_COMPONENT_TYPE_TABLE) {
        const TableData& rTable = rComponent.tableData;
        // 计算行列数
        const int nRows = rTable.vecRows.size();
        int nCols = 1;
        for (const QStringList& rRow : rTable.vecRows) {
            nCols = qMax(nCols, rRow.size());
        }
        if (nRows == 0) {
            pPainter->setPen(QPen(rTable.borderColor, 1));
            pPainter->setBrush(Qt::NoBrush);
            pPainter->drawRect(rContentRect);
            return;
        }
        const qreal dCellWidth = rContentRect.width() / nCols;
        const qreal dCellHeight = rContentRect.height() / nRows;
        QFont font(QStringLiteral("Microsoft YaHei"));
        font.setPixelSize(rTable.nFontSize);
        pPainter->setFont(font);

        for (int nRow = 0; nRow < nRows; ++nRow) {
            const QStringList& rRowData = rTable.vecRows.at(nRow);
            for (int nCol = 0; nCol < nCols; ++nCol) {
                const QRectF cellRect(rContentRect.x() + nCol * dCellWidth,
                                      rContentRect.y() + nRow * dCellHeight,
                                      dCellWidth, dCellHeight);
                // 背景：表头 / 斑马纹 / 白
                QColor fillColor(Qt::white);
                if (nRow == 0 && rTable.bShowHeader) {
                    fillColor = rTable.headerColor;
                } else if (rTable.bAlternateRow && (nRow % 2 == 0)) {
                    fillColor = QColor(245, 245, 245);
                }
                pPainter->fillRect(cellRect, fillColor);
                pPainter->setPen(QPen(rTable.borderColor, 1));
                pPainter->drawRect(cellRect);
                // 文本
                pPainter->setPen(rTable.textColor);
                const QString strCell = nCol < rRowData.size() ? rRowData.at(nCol) : QString();
                pPainter->drawText(cellRect.adjusted(4, 0, -4, 0),
                                   Qt::AlignLeft | Qt::AlignVCenter, strCell);
            }
        }
        return;
    }

    if (rComponent.eType == E_COMPONENT_TYPE_STICKER) {
        const StickerData& rSticker = rComponent.stickerData;
        const QColor color = rSticker.color;
        const QRectF rect = rContentRect;
        switch (rSticker.eStickerType) {
        case E_STICKER_TYPE_TITLE_LINE: {
            // 标题装饰线：左侧竖条 + 右侧横线
            pPainter->setPen(Qt::NoPen);
            pPainter->setBrush(color);
            pPainter->drawRect(QRectF(rect.x(), rect.y(), 8, rect.height()));
            pPainter->drawRect(QRectF(rect.x() + 16, rect.y() + rect.height() / 2 - 2,
                                      rect.width() - 16, 4));
            break;
        }
        case E_STICKER_TYPE_CORNER_BADGE: {
            // 左上角三角旗标
            QPainterPath path;
            path.moveTo(rect.x(), rect.y());
            path.lineTo(rect.x() + rect.width(), rect.y());
            path.lineTo(rect.x(), rect.y() + rect.height());
            path.closeSubpath();
            pPainter->fillPath(path, color);
            break;
        }
        case E_STICKER_TYPE_STAR_RATING: {
            // 5 颗星
            pPainter->setPen(Qt::NoPen);
            pPainter->setBrush(color);
            const qreal dStarSize = qMin(rect.width() / 5, rect.height());
            for (int nIndex = 0; nIndex < 5; ++nIndex) {
                const QPointF center(rect.x() + nIndex * dStarSize + dStarSize / 2,
                                     rect.y() + rect.height() / 2);
                drawStar(pPainter, center, dStarSize / 2);
            }
            break;
        }
        case E_STICKER_TYPE_ARROW: {
            // 右向箭头
            pPainter->setPen(QPen(color, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            pPainter->setBrush(color);
            const QPointF end(rect.right() - 12, rect.center().y());
            pPainter->drawLine(QPointF(rect.left(), rect.center().y()), end);
            QPainterPath head;
            head.moveTo(rect.right(), rect.center().y());
            head.lineTo(end.x(), rect.top() + 6);
            head.lineTo(end.x(), rect.bottom() - 6);
            head.closeSubpath();
            pPainter->fillPath(head, color);
            break;
        }
        case E_STICKER_TYPE_DIVIDER: {
            // 分割线：中段线 + 两端圆点
            pPainter->setPen(QPen(color, 3));
            pPainter->drawLine(QPointF(rect.left() + 6, rect.center().y()),
                               QPointF(rect.right() - 6, rect.center().y()));
            pPainter->setPen(Qt::NoPen);
            pPainter->setBrush(color);
            pPainter->drawEllipse(QPointF(rect.left() + 6, rect.center().y()), 4, 4);
            pPainter->drawEllipse(QPointF(rect.right() - 6, rect.center().y()), 4, 4);
            break;
        }
        case E_STICKER_TYPE_CARD_BORDER:
        default: {
            // 卡片边框：外框 + 内框
            pPainter->setPen(QPen(color, 3));
            pPainter->setBrush(Qt::NoBrush);
            pPainter->drawRoundedRect(rect, 8, 8);
            pPainter->setPen(QPen(color, 1));
            pPainter->drawRoundedRect(rect.adjusted(6, 6, -6, -6), 5, 5);
            break;
        }
        }
        return;
    }

    // 形状
    const ShapeData& rShape = rComponent.shapeData;
    QPainterPath path;
    switch (rShape.eShapeType) {
    case E_SHAPE_TYPE_ROUND_RECT:
        path.addRoundedRect(rContentRect, 12, 12);
        break;
    case E_SHAPE_TYPE_ELLIPSE:
        path.addEllipse(rContentRect);
        break;
    case E_SHAPE_TYPE_LINE: {
        pPainter->setPen(QPen(rShape.borderColor, rShape.nBorderWidth));
        pPainter->drawLine(rContentRect.topLeft(), rContentRect.bottomRight());
        return;
    }
    case E_SHAPE_TYPE_RECTANGLE:
    default:
        path.addRect(rContentRect);
        break;
    }
    pPainter->fillPath(path, rShape.fillColor);
    pPainter->strokePath(path, QPen(rShape.borderColor, rShape.nBorderWidth));
}

} // namespace bwm
