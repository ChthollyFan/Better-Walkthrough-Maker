/**
 * @file ImageFit.h
 * @author zhangweimu
 * @brief 图片适配绘制工具：等比覆盖（cover）取景计算，支持取景偏移。
 *
 * 页面背景图（core/PageBackground）与卡片边框内的图片共用本实现，
 * 保证「框内图片」与「页面背景图」的缩放裁剪规则完全一致。
 */
#ifndef BWM_CORE_IMAGEFIT_H
#define BWM_CORE_IMAGEFIT_H

#include <QRectF>
#include <QSizeF>

class QImage;
class QPainter;

namespace bwm {

/**
 * @brief 图片适配工具（静态方法集合）。
 */
class ImageFit
{
public:
    /**
     * @brief 计算「等比覆盖」所需的图片源矩形（图片像素坐标）。
     *
     * 缩放倍率取宽、高比例中的较大者：图片铺满目标矩形，比例不符时裁剪多余部分，
     * 既不拉伸变形也不留白。
     *
     * @param rImageSize  图片尺寸
     * @param rTargetRect 目标矩形（框）
     * @param dOffsetX    水平取景位置：0 = 贴左边缘，0.5 = 居中，1 = 贴右边缘
     * @param dOffsetY    垂直取景位置：0 = 贴上边缘，0.5 = 居中，1 = 贴下边缘
     * @return            源矩形；参数非法时返回空矩形
     */
    static QRectF coverSourceRect(const QSizeF& rImageSize, const QRectF& rTargetRect,
                                  qreal dOffsetX = 0.5, qreal dOffsetY = 0.5);

    /**
     * @brief 以「等比覆盖、超出裁剪」方式把图片绘制到 rTargetRect（含取景偏移）。
     *
     * 图片为空或目标矩形为空时不绘制。
     *
     * @param pPainter    绘制器
     * @param rImage      源图片
     * @param rTargetRect 目标矩形
     * @param dOffsetX    水平取景位置（0~1）
     * @param dOffsetY    垂直取景位置（0~1）
     */
    static void paintCover(QPainter* pPainter, const QImage& rImage, const QRectF& rTargetRect,
                           qreal dOffsetX = 0.5, qreal dOffsetY = 0.5);
};

} // namespace bwm

#endif // BWM_CORE_IMAGEFIT_H
