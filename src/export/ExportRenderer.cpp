/**
 * @file ExportRenderer.cpp
 * @author zhangweimu
 * @brief PNG 导出渲染实现。
 */
#include "export/ExportRenderer.h"

#include <QImage>
#include <QPainter>

#include <algorithm>

#include "core/ComponentPainter.h"
#include "core/PageBackground.h"
#include "project/AssetStore.h"

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

// 绘制页面背景图（等比覆盖铺满整页）；dOffsetY 为长图拼接时的纵向偏移。
// 页面无背景图或图片加载失败时不绘制，保留已填充的主题背景色。
void paintPageBackground(QPainter* pPainter, const Page& rPage, qreal dOffsetY,
                         const QString& rProjectDirectory)
{
    if (rPage.strBackgroundImage.isEmpty()) {
        return;
    }
    const QString strImagePath = AssetStore::resolvePath(rPage.strBackgroundImage,
                                                         rProjectDirectory);
    const QImage image(strImagePath);
    if (image.isNull()) {
        return;
    }
    PageBackground::paintCover(pPainter, image,
                               QRectF(0, dOffsetY, rPage.size.width(), rPage.size.height()));
}

// 在指定原点绘制一页的全部组件（y 偏移用于长图拼接）
// strProjectDirectory 用于解析组件内图片的相对路径（卡片边框图片）
void paintPageComponents(QPainter* pPainter, const Page& rPage, qreal dOffsetY,
                         const QString& strProjectDirectory)
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
                                QRectF(QPointF(0, 0), rComponent.size), nullptr,
                                strProjectDirectory);
        pPainter->restore();
    }
}

} // namespace

// 按样式在图片指定角落绘制作者署名（支持位置/字体/字号/加粗/颜色/不透明度）
void ExportRenderer::drawAuthorMark(QImage& rImage, const QString& rAuthor,
                                    const AuthorMarkStyle& rStyle, qreal dScaleFactor)
{
    if (rAuthor.trimmed().isEmpty() || rImage.isNull()) {
        return;
    }

    // 字号：逻辑像素 × 缩放系数，下限 12px 保证小图上仍可辨认
    QFont font(rStyle.resolvedFontFamily());
    font.setPixelSize(qMax(12, qRound(rStyle.nFontSize * dScaleFactor)));
    font.setBold(rStyle.bBold);

    // 颜色：仅 RGB 存设置，不透明度在绘制时合成到 alpha
    QColor color = rStyle.color;
    color.setAlphaF(qBound(0, rStyle.nOpacityPercent, 100) / 100.0);

    QPainter painter(&rImage);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(font);
    painter.setPen(color);

    // 内边距沿用历史值（水平 16、垂直 10 逻辑像素）并按倍率缩放；
    // 取图片尺寸的 1/4 作为上限，避免小图上文字矩形反向导致不绘制。
    const qreal dMarginX = qMin(16.0 * dScaleFactor, rImage.width() / 4.0);
    const qreal dMarginY = qMin(10.0 * dScaleFactor, rImage.height() / 4.0);
    const QRectF textRect = QRectF(QPointF(0, 0), QSizeF(rImage.size()))
                                .adjusted(dMarginX, dMarginY, -dMarginX, -dMarginY);

    // 四角对齐方式
    int nAlignment = Qt::AlignLeft | Qt::AlignTop;
    switch (rStyle.ePosition) {
    case E_AUTHOR_MARK_POSITION_TOP_LEFT:
        nAlignment = Qt::AlignLeft | Qt::AlignTop;
        break;
    case E_AUTHOR_MARK_POSITION_TOP_RIGHT:
        nAlignment = Qt::AlignRight | Qt::AlignTop;
        break;
    case E_AUTHOR_MARK_POSITION_BOTTOM_LEFT:
        nAlignment = Qt::AlignLeft | Qt::AlignBottom;
        break;
    case E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT:
    default:
        nAlignment = Qt::AlignRight | Qt::AlignBottom;
        break;
    }

    painter.drawText(textRect, nAlignment, QStringLiteral("by %1").arg(rAuthor));
}

QImage ExportRenderer::renderPage(const Page& rPage, qreal dScale, const QColor& rBackground,
                                  const QString& rAuthor, const QString& rProjectDirectory,
                                  const AuthorMarkStyle& rAuthorStyle)
{
    const int nWidth = qMax(1, qRound(rPage.size.width() * dScale));
    const int nHeight = qMax(1, qRound(rPage.size.height() * dScale));
    QImage image(nWidth, nHeight, QImage::Format_ARGB32_Premultiplied);
    image.fill(rBackground);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.scale(dScale, dScale);
    // 先铺页面背景图，再画组件（与画布层序一致：背景图在最底）
    paintPageBackground(&painter, rPage, 0, rProjectDirectory);
    paintPageComponents(&painter, rPage, 0, rProjectDirectory);
    painter.end();

    // 署名水印按导出倍率缩放（与页面内容同步放大）
    drawAuthorMark(image, rAuthor, rAuthorStyle, dScale);
    return image;
}

QImage ExportRenderer::renderLongImage(const QVector<Page>& rPages, qreal dScale, bool bSeparator,
                                       QString* pErrorMessage, const QColor& rBackground,
                                       const QString& rAuthor, const QString& rProjectDirectory,
                                       const AuthorMarkStyle& rAuthorStyle)
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
        paintPageBackground(&painter, rPage, dOffsetY, rProjectDirectory);
        paintPageComponents(&painter, rPage, dOffsetY, rProjectDirectory);
        dOffsetY += rPage.size.height();
        if (bSeparator && nIndex < rPages.size() - 1) {
            painter.fillRect(QRectF(0, dOffsetY, nWidth, nSeparatorHeight), QColor(200, 200, 200));
            dOffsetY += nSeparatorHeight;
        }
    }
    painter.end();

    // 署名水印按导出倍率缩放
    drawAuthorMark(image, rAuthor, rAuthorStyle, dScale);
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
