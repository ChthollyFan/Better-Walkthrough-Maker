/**
 * @file ImageFit.cpp
 * @author zhangweimu
 * @brief 图片适配绘制实现（等比覆盖 + 取景偏移）。
 */
#include "core/ImageFit.h"

#include <QImage>
#include <QPainter>
#include <QtGlobal>

namespace bwm {

QRectF ImageFit::coverSourceRect(const QSizeF& rImageSize, const QRectF& rTargetRect,
                                 qreal dOffsetX, qreal dOffsetY)
{
    if (rImageSize.width() <= 0 || rImageSize.height() <= 0
        || rTargetRect.width() <= 0 || rTargetRect.height() <= 0) {
        return QRectF();
    }
    // 覆盖式缩放：取宽高比例中的较大者，保证两个方向都不小于目标矩形
    const qreal dScale = qMax(rTargetRect.width() / rImageSize.width(),
                              rTargetRect.height() / rImageSize.height());
    // 目标矩形在图片坐标系下的尺寸
    const QSizeF sourceSize(rTargetRect.width() / dScale, rTargetRect.height() / dScale);
    // 取景偏移：可移动范围为「图片尺寸 - 取景窗口尺寸」，0 贴左/上、1 贴右/下。
    // 偏移越界（外部数据异常）时夹到合法区间，避免取到图片外的空白。
    const qreal dClampedX = qBound(0.0, dOffsetX, 1.0);
    const qreal dClampedY = qBound(0.0, dOffsetY, 1.0);
    return QRectF(dClampedX * (rImageSize.width() - sourceSize.width()),
                  dClampedY * (rImageSize.height() - sourceSize.height()),
                  sourceSize.width(), sourceSize.height());
}

void ImageFit::paintCover(QPainter* pPainter, const QImage& rImage, const QRectF& rTargetRect,
                          qreal dOffsetX, qreal dOffsetY)
{
    if (!pPainter || rImage.isNull()) {
        return;
    }
    const QRectF sourceRect = coverSourceRect(QSizeF(rImage.size()), rTargetRect,
                                              dOffsetX, dOffsetY);
    if (sourceRect.isEmpty()) {
        return;
    }
    pPainter->drawImage(rTargetRect, rImage, sourceRect);
}

} // namespace bwm
