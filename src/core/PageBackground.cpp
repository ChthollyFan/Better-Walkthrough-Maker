/**
 * @file PageBackground.cpp
 * @author zhangweimu
 * @brief 页面背景图绘制实现（等比覆盖铺满，居中取景）。
 *
 * 具体算法统一由 core/ImageFit 提供：页面背景图与卡片边框内的图片共用同一实现，
 * 本类只保留「页面背景」语义与居中取景的默认参数。
 */
#include "core/PageBackground.h"

#include "core/ImageFit.h"

#include <QImage>
#include <QPainter>

namespace bwm {

QRectF PageBackground::coverSourceRect(const QSizeF& rImageSize, const QRectF& rTargetRect)
{
    return ImageFit::coverSourceRect(rImageSize, rTargetRect, 0.5, 0.5);
}

void PageBackground::paintCover(QPainter* pPainter, const QImage& rImage, const QRectF& rTargetRect)
{
    ImageFit::paintCover(pPainter, rImage, rTargetRect, 0.5, 0.5);
}

} // namespace bwm
