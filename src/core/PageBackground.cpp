/**
 * @file PageBackground.cpp
 * @author zhangweimu
 * @brief 页面背景图绘制实现（等比覆盖铺满）。
 */
#include "core/PageBackground.h"

#include <QImage>
#include <QPainter>

namespace bwm {

QRectF PageBackground::coverSourceRect(const QSizeF& rImageSize, const QRectF& rTargetRect)
{
    if (rImageSize.width() <= 0 || rImageSize.height() <= 0
        || rTargetRect.width() <= 0 || rTargetRect.height() <= 0) {
        return QRectF();
    }
    // 覆盖式缩放：取宽高比例中的较大者，保证两个方向都不小于目标矩形
    const qreal dScale = qMax(rTargetRect.width() / rImageSize.width(),
                              rTargetRect.height() / rImageSize.height());
    // 目标矩形在图片坐标系下的尺寸，居中裁剪
    const QSizeF sourceSize(rTargetRect.width() / dScale, rTargetRect.height() / dScale);
    return QRectF((rImageSize.width() - sourceSize.width()) / 2.0,
                  (rImageSize.height() - sourceSize.height()) / 2.0,
                  sourceSize.width(), sourceSize.height());
}

void PageBackground::paintCover(QPainter* pPainter, const QImage& rImage, const QRectF& rTargetRect)
{
    if (!pPainter || rImage.isNull()) {
        return;
    }
    const QRectF sourceRect = coverSourceRect(QSizeF(rImage.size()), rTargetRect);
    if (sourceRect.isEmpty()) {
        return;
    }
    pPainter->drawImage(rTargetRect, rImage, sourceRect);
}

} // namespace bwm
