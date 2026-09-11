/**
 * @file ComponentPainter.h
 * @author zhangweimu
 * @brief 组件统一绘制函数：编辑器渲染与 PNG 导出共用同一实现（所见即所得）。
 */
#ifndef BWM_CORE_COMPONENTPAINTER_H
#define BWM_CORE_COMPONENTPAINTER_H

#include <QRectF>
#include <QString>

#include "core/Component.h"

class QImage;
class QPainter;

namespace bwm {

// 组件统一绘制：根据组件类型把内容画到指定矩形内。
// 编辑器中带图片缓存（避免每帧加载），导出时传 nullptr（一次性加载）。
class ComponentPainter {
public:
    // pImageCache：单张图片缓存（图片组件与卡片边框图片共用；传 nullptr 表示每次重新加载）。
    // strProjectDirectory：项目目录，用于把卡片边框图片的相对路径解析为绝对路径。
    static void paint(QPainter* pPainter, const Component& rComponent,
                      const QRectF& rContentRect, QImage* pImageCache = nullptr,
                      const QString& strProjectDirectory = QString());
};

} // namespace bwm

#endif // BWM_CORE_COMPONENTPAINTER_H
