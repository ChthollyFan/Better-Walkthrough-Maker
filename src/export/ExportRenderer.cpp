/**
 * @file ExportRenderer.cpp
 * @author zhangweimu
 * @brief PNG 导出渲染实现。
 */
#include "export/ExportRenderer.h"

#include <QPainter>

#include <algorithm>

#include "core/ComponentPainter.h"

namespace bwm {

namespace {

// 长图总高上限（逻辑像素）。QImage 尺寸受限（约 32767px），
// 此处保守设 30000，超出时提示分片导出（规划第 9 节风险对策）。
constexpr qint64 nMaxLongImageHeight = 30000;
// 长图页间分隔线高度（逻辑像素）
constexpr qint64 nSeparatorHeight = 20;

// 按 zOrder 升序排列组件（保证绘制顺序与图层面板一致）
QVector<Component> sortedComponents(const QVector<Component>& rComponents)
{
    QVector<Component> sorted = rComponents;
    std::sort(sorted.begin(), sorted.end(),
              [](const Component& rLeft, const Component& rRight) {
                  return rLeft.nZOrder < rRight.nZOrder;
              });
    return sorted;
}

// 在指定原点绘制一页的全部组件（y 偏移用于长图拼接）
void paintPageComponents(QPainter* pPainter, const Page& rPage, qreal dOffsetY)
{
    const QVector<Component> sorted = sortedComponents(rPage.vecComponents);
    for (const Component& rComponent : sorted) {
        if (!rComponent.bVisible) {
            continue;
        }
        pPainter->save();
        pPainter->translate(rComponent.pos.x(), dOffsetY + rComponent.pos.y());
        pPainter->rotate(rComponent.dRotation);
        ComponentPainter::paint(pPainter, rComponent,
                                QRectF(QPointF(0, 0), rComponent.size));
        pPainter->restore();
    }
}

} // namespace

QImage ExportRenderer::renderPage(const Page& rPage, qreal dScale, const QColor& rBackground)
{
    const int nWidth = qMax(1, qRound(rPage.size.width() * dScale));
    const int nHeight = qMax(1, qRound(rPage.size.height() * dScale));
    QImage image(nWidth, nHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(rBackground);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.scale(dScale, dScale);
    paintPageComponents(&painter, rPage, 0);
    return image;
}

QImage ExportRenderer::renderLongImage(const QVector<Page>& rPages, qreal dScale, bool bSeparator,
                                       QString* pErrorMessage, const QColor& rBackground)
{
    if (rPages.isEmpty()) {
        return QImage();
    }

    // 计算画布尺寸：宽度取最大页宽，高度为各页高之和（可选分隔线）
    int nWidth = 0;
    qint64 nTotalHeight = 0;
    for (int nIndex = 0; nIndex < rPages.size(); ++nIndex) {
        nWidth = qMax(nWidth, rPages.at(nIndex).size.width());
        nTotalHeight += rPages.at(nIndex).size.height();
        if (bSeparator && nIndex < rPages.size() - 1) {
            nTotalHeight += nSeparatorHeight;
        }
    }

    if (nTotalHeight > nMaxLongImageHeight) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("长图总高度过大（%1px，上限 %2px），请减少页数或拆分导出")
                                .arg(nTotalHeight).arg(nMaxLongImageHeight);
        }
        return QImage();
    }

    QImage image(qMax(1, qRound(nWidth * dScale)), qMax(1, qRound(nTotalHeight * dScale)),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(rBackground);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.scale(dScale, dScale);

    qreal dOffsetY = 0;
    for (int nIndex = 0; nIndex < rPages.size(); ++nIndex) {
        const Page& rPage = rPages.at(nIndex);
        // 页面背景（宽度不足最大宽时补背景色）
        painter.fillRect(QRectF(0, dOffsetY, nWidth, rPage.size.height()), rBackground);
        paintPageComponents(&painter, rPage, dOffsetY);
        dOffsetY += rPage.size.height();
        if (bSeparator && nIndex < rPages.size() - 1) {
            painter.fillRect(QRectF(0, dOffsetY, nWidth, nSeparatorHeight), QColor(200, 200, 200));
            dOffsetY += nSeparatorHeight;
        }
    }
    return image;
}

bool ExportRenderer::writePng(const QImage& rImage, const QString& strFilePath, QString* pErrorMessage)
{
    if (rImage.isNull()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("导出图片为空");
        }
        return false;
    }
    if (!rImage.save(strFilePath, "PNG")) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("写入文件失败：%1").arg(strFilePath);
        }
        return false;
    }
    return true;
}

} // namespace bwm
