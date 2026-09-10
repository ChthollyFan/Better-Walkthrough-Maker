/**
 * @file ArticleRenderer.cpp
 * @author zhangweimu
 * @brief 文章渲染辅助工具实现。
 */
#include "export/ArticleRenderer.h"

#include "core/Article.h"
#include "core/Project.h"
#include "export/ExportRenderer.h"
#include "plugin/PluginContext.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextImageFormat>
#include <QUrl>

namespace bwm {

// 页面引用正则
static const QRegularExpression kPageRefRegex(kPageRefPattern);
// 普通图片语法正则
static const QRegularExpression kImageRegex(QStringLiteral(R"(!\[([^\]]*)\]\(([^)]+)\))"));

QTextDocument* ArticleRenderer::buildDocument(const Article& rArticle,
                                               const Project& rProject,
                                               const PluginContext& rContext,
                                               int nImageWidth)
{
    auto* pDocument = new QTextDocument;

    // 第一步：解析页面引用 ![[W:P]]，替换为 ![](bwm://page/W/P)
    QString strResolved = rArticle.strMarkdown;
    QVector<QPair<int, int>> vecPageRefs;

    QRegularExpressionMatchIterator it = kPageRefRegex.globalMatch(rArticle.strMarkdown);
    QVector<QRegularExpressionMatch> vecMatches;
    while(it.hasNext()) {
        vecMatches.append(it.next());
    }
    for(int i = vecMatches.size() - 1; i >= 0; --i) {
        const QRegularExpressionMatch& rMatch = vecMatches.at(i);
        const int nW = rMatch.captured(1).toInt();
        const int nP = rMatch.captured(2).toInt();
        const QUrl resourceUrl(QStringLiteral("bwm://page/%1/%2").arg(nW).arg(nP));
        const QString strImageMarkdown = QStringLiteral("![页面%1:%2](%3)")
                                             .arg(nW).arg(nP).arg(resourceUrl.toString());
        strResolved.replace(rMatch.capturedStart(), rMatch.capturedLength(), strImageMarkdown);
        vecPageRefs.append({nW, nP});
    }

    // 第二步：setMarkdown 渲染
    // 设置默认样式表：文字颜色跟随主题（否则深色主题下默认黑字不可见），
    // 字号按长图宽度缩放（默认 12pt 在 1080px 宽的图上过小）。
    // 字号与图片宽度成比例：1080 宽 → 正文 44px（一行约 24 字，手机阅读舒适）
    const int nBodySize = qMax(16, nImageWidth * 44 / 1080);
    pDocument->setDefaultStyleSheet(
        QStringLiteral("body { color: %1; font-size: %2px; }"
                       "h1 { font-size: %3px; }"
                       "h2 { font-size: %4px; }"
                       "h3 { font-size: %5px; }"
                       "h4, h5, h6 { font-size: %6px; }"
                       "code, pre { font-size: %7px; }")
            .arg(rContext.theme.textColor.name())
            .arg(nBodySize)
            .arg(nBodySize * 16 / 10)      // h1 ≈ 1.6x
            .arg(nBodySize * 13 / 10)      // h2 ≈ 1.3x
            .arg(nBodySize * 11 / 10)      // h3 ≈ 1.1x
            .arg(nBodySize)                // h4-h6 同正文
            .arg(nBodySize * 9 / 10));     // 代码块略小
    // 默认字体作为兜底（CSS 未覆盖的部分）
    QFont defaultFont = pDocument->defaultFont();
    defaultFont.setPixelSize(nBodySize);
    pDocument->setDefaultFont(defaultFont);
    pDocument->setMarkdown(strResolved);

    // 第三步：注册页面引用图片 resource
    for(const auto& rRef : vecPageRefs) {
        const QImage image = renderPageRef(rProject, rRef.first, rRef.second,
                                           rContext.theme.backgroundColor, 1.0,
                                           rContext.projectDirectory);
        if(!image.isNull()) {
            const QUrl resourceUrl(QStringLiteral("bwm://page/%1/%2")
                                       .arg(rRef.first).arg(rRef.second));
            pDocument->addResource(QTextDocument::ImageResource, resourceUrl, image);
        }
    }

    // 第四步：遍历 document 中所有图片，加载普通图片并注册，限制图片宽度
    for(QTextBlock block = pDocument->firstBlock(); block.isValid(); block = block.next()) {
        for(QTextBlock::iterator blockIt = block.begin(); !blockIt.atEnd(); ++blockIt) {
            QTextFragment fragment = blockIt.fragment();
            if(!fragment.isValid()) {
                continue;
            }
            QTextCharFormat charFormat = fragment.charFormat();
            if(!charFormat.isImageFormat()) {
                continue;
            }
            QTextImageFormat imageFormat = charFormat.toImageFormat();
            const QString strName = imageFormat.name();
            const QUrl url(strName);

            // 跳过页面引用图片（bwm:// 协议已注册）
            if(url.scheme() != QStringLiteral("bwm")) {
                QString strPath = url.path();
                if(strPath.isEmpty()) {
                    strPath = strName;
                }
                QFileInfo info(strPath);
                if(info.isRelative() && !rContext.projectDirectory.isEmpty()) {
                    strPath = QDir(rContext.projectDirectory).filePath(strPath);
                }
                QFile file(strPath);
                if(file.open(QIODevice::ReadOnly)) {
                    QImage image;
                    if(image.loadFromData(file.readAll())) {
                        pDocument->addResource(QTextDocument::ImageResource, url, image);
                    }
                }
            }

            // 限制图片最大宽度
            if(nImageWidth > 0) {
                const int nCurrentWidth = imageFormat.width();
                if(nCurrentWidth <= 0 || nCurrentWidth > nImageWidth) {
                    QTextCursor cursor(block);
                    cursor.setPosition(fragment.position());
                    cursor.setPosition(fragment.position() + fragment.length(),
                                        QTextCursor::KeepAnchor);
                    imageFormat.setWidth(nImageWidth);
                    cursor.setCharFormat(imageFormat);
                }
            }
        }
    }

    // 第五步：统一设置文字颜色为主题文字色（兜底，确保深色主题下文字可见）
    for(QTextBlock block = pDocument->firstBlock(); block.isValid(); block = block.next()) {
        for(QTextBlock::iterator blockIt = block.begin(); !blockIt.atEnd(); ++blockIt) {
            QTextFragment fragment = blockIt.fragment();
            if(!fragment.isValid() || fragment.charFormat().isImageFormat()) {
                continue;   // 跳过图片，只处理文字
            }
            QTextCursor cursor(pDocument);
            cursor.setPosition(fragment.position());
            cursor.setPosition(fragment.position() + fragment.length(),
                                QTextCursor::KeepAnchor);
            QTextCharFormat charFormat;
            charFormat.setForeground(rContext.theme.textColor);
            cursor.mergeCharFormat(charFormat);
        }
    }

    return pDocument;
}

QImage ArticleRenderer::renderPageRef(const Project& rProject, int nW, int nP,
                                       const QColor& rBackground, qreal dScale,
                                       const QString& strProjectDirectory)
{
    if(nW < 0 || nW >= rProject.vecWalkthroughs.size()) {
        return QImage();
    }
    const Walkthrough& rWalkthrough = rProject.vecWalkthroughs.at(nW);
    if(nP < 0 || nP >= rWalkthrough.vecPages.size()) {
        return QImage();
    }
    // 传入项目目录：页面带背景图时按相对路径解析（否则背景图会缺失）
    return ExportRenderer::renderPage(rWalkthrough.vecPages.at(nP), dScale, rBackground,
                                      QString(), strProjectDirectory);
}

QString ArticleRenderer::sanitizeFileName(const QString& strName)
{
    QString strSafe = strName;
    strSafe.replace(QRegularExpression(QStringLiteral(R"([\\/:*?\"<>|])")),
                  QStringLiteral("_"));
    return strSafe;
}

} // namespace bwm
