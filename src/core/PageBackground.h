/**
 * @file PageBackground.h
 * @author zhangweimu
 * @brief 页面背景图绘制：等比覆盖（cover）铺满整页。
 *
 * 画布（editor/CanvasScene）与导出渲染（export/ExportRenderer）共用同一套
 * 绘制算法，保证「所见即所得」——编辑器里看到的背景图与导出的 PNG 完全一致。
 */
#ifndef BWM_CORE_PAGEBACKGROUND_H
#define BWM_CORE_PAGEBACKGROUND_H

#include <QRectF>
#include <QSizeF>

class QImage;
class QPainter;

namespace bwm {

/**
 * @brief 页面背景图绘制工具（静态方法集合）。
 */
class PageBackground
{
public:
    /**
     * @brief 计算「等比覆盖」所需的图片源矩形（图片像素坐标）。
     *
     * 缩放倍率取宽、高比例的较大者：图片铺满目标矩形，比例不符时居中裁剪多余部分，
     * 既不拉伸变形也不留白。
     *
     * @param rImageSize   图片尺寸
     * @param rTargetRect  目标矩形（页面矩形）
     * @return             源矩形；参数非法时返回空矩形
     */
    static QRectF coverSourceRect(const QSizeF& rImageSize, const QRectF& rTargetRect);

    /**
     * @brief 以「等比覆盖、超出裁剪」方式把图片绘制到 rTargetRect。
     *
     * 图片为空或目标矩形为空时不绘制（保持调用方已填充的页面背景色）。
     *
     * @param pPainter     绘制器
     * @param rImage       背景图
     * @param rTargetRect  目标矩形（页面矩形）
     */
    static void paintCover(QPainter* pPainter, const QImage& rImage, const QRectF& rTargetRect);
};

} // namespace bwm

#endif // BWM_CORE_PAGEBACKGROUND_H
