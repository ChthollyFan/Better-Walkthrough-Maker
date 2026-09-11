/**
 * @file ExportRenderer.h
 * @author zhangweimu
 * @brief PNG 导出渲染：单页 / 长图（复用 ComponentPainter，与编辑器所见即所得）。
 */
#ifndef BWM_EXPORT_EXPORTRENDERER_H
#define BWM_EXPORT_EXPORTRENDERER_H

#include <QImage>
#include <QVector>

#include "core/AuthorMarkStyle.h"
#include "core/Project.h"

namespace bwm {

// PNG 导出渲染：按倍率把页面渲染为 QImage（长图纵向拼接）。
class ExportRenderer {
public:
    // 渲染单页（dScale 为倍率；rBackground 为页面背景色，跟随主题；
    // rAuthor 非空时按 rAuthorStyle 在指定角落标注 by 作者名；
    // rProjectDirectory 非空时用于把页面背景图的相对路径解析为绝对路径）
    static QImage renderPage(const Page& rPage, qreal dScale,
                             const QColor& rBackground = Qt::white,
                             const QString& rAuthor = QString(),
                             const QString& rProjectDirectory = QString(),
                             const AuthorMarkStyle& rAuthorStyle = AuthorMarkStyle());
    // 渲染多页为长图：纵向拼接，bSeparator 时页间绘制分隔线
    // （rProjectDirectory / rAuthorStyle 含义同 renderPage）
    static QImage renderLongImage(const QVector<Page>& rPages, qreal dScale, bool bSeparator,
                                  QString* pErrorMessage, const QColor& rBackground = Qt::white,
                                  const QString& rAuthor = QString(),
                                  const QString& rProjectDirectory = QString(),
                                  const AuthorMarkStyle& rAuthorStyle = AuthorMarkStyle());
    // 写 PNG 文件；失败时返回 false 并给出原因
    static bool writePng(const QImage& rImage, const QString& strFilePath, QString* pErrorMessage);

    // 按样式在图片指定角落绘制作者署名；rAuthor 为空时不绘制。
    // rStyle 决定位置/字体族/字号/加粗/颜色/不透明度，rStyle 为默认值时等价于旧版行为
    // （右下角、微软雅黑、18px、黑色 63% 不透明）。
    // dScaleFactor 为字号与边距的缩放系数：页面导出传导出倍率，文章长图传 图片宽度/720。
    // 供文章导出等自定义渲染流程复用。
    static void drawAuthorMark(QImage& rImage, const QString& rAuthor,
                               const AuthorMarkStyle& rStyle = AuthorMarkStyle(),
                               qreal dScaleFactor = 1.0);
};

} // namespace bwm

#endif // BWM_EXPORT_EXPORTRENDERER_H
