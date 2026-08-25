/**
 * @file ExportRenderer.h
 * @author zhangweimu
 * @brief PNG 导出渲染：单页 / 长图（复用 ComponentPainter，与编辑器所见即所得）。
 */
#ifndef BWM_EXPORT_EXPORTRENDERER_H
#define BWM_EXPORT_EXPORTRENDERER_H

#include <QImage>
#include <QVector>

#include "core/Project.h"

namespace bwm {

// PNG 导出渲染：按倍率把页面渲染为 QImage（长图纵向拼接）。
class ExportRenderer {
public:
    // 渲染单页（dScale 为倍率，如 2.0；rBackground 为页面背景色，跟随主题）
    static QImage renderPage(const Page& rPage, qreal dScale,
                             const QColor& rBackground = Qt::white);
    // 渲染多页为长图：纵向拼接，bSeparator 时页间绘制分隔线
    static QImage renderLongImage(const QVector<Page>& rPages, qreal dScale, bool bSeparator,
                                  QString* pErrorMessage, const QColor& rBackground = Qt::white);
    // 写 PNG 文件；失败时返回 false 并给出原因
    static bool writePng(const QImage& rImage, const QString& strFilePath, QString* pErrorMessage);
};

} // namespace bwm

#endif // BWM_EXPORT_EXPORTRENDERER_H
