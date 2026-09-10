/**
 * @file MarkdownPreview.cpp
 * @author zhangweimu
 * @brief Markdown 预览控件实现。
 *
 * 图片渲染机制：
 * - setMarkdown 后遍历 document 中所有 QTextImageFormat，获取实际 URL，
 *   按 URL 加载图片并注册到 document 的 ImageResource。
 * - 页面引用 ![[W:P]] 先替换为 ![](bwm://page/W/P)，setMarkdown 后渲染页面并注册。
 * - 大图片（含页面引用）限制最大显示宽度为预览区宽度，避免撑满。
 */
#include "app/panels/MarkdownPreview.h"

#include "core/Article.h"
#include "core/Project.h"
#include "export/ExportRenderer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextImageFormat>
#include <QUrl>
#include <QWheelEvent>

namespace bwm {

// 页面引用正则：匹配 ![[W:P]]，捕获组 1=W，捕获组 2=P
static const QRegularExpression kPageRefRegex(kPageRefPattern);

MarkdownPreview::MarkdownPreview(QWidget* pParent)
    : QTextBrowser(pParent)
{
    setOpenExternalLinks(true);
    setReadOnly(true);
    // 开启文字/图片随控件宽度自动换行
    setLineWrapMode(QTextBrowser::WidgetWidth);
}

void MarkdownPreview::setProjectDirectory(const QString& strDir)
{
    m_strProjectDirectory = strDir;
}

void MarkdownPreview::setProject(const Project* pProject, const QColor& rBackgroundColor)
{
    m_pProject = pProject;
    m_backgroundColor = rBackgroundColor;
}

void MarkdownPreview::setMarkdownSource(const QString& strMarkdown)
{
    m_strSource = strMarkdown;
    renderContent();
}

void MarkdownPreview::renderContent()
{
    // 第一步：解析页面引用 ![[W:P]]，替换为 ![](bwm://page/W/P) 图片语法
    QString strResolved = m_strSource;
    QVector<QPair<int, int>> vecPageRefs;

    if(m_pProject) {
        QRegularExpressionMatchIterator it = kPageRefRegex.globalMatch(m_strSource);
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
    }

    // 第二步：setMarkdown 渲染（重建 document 内容）
    document()->setMarkdown(strResolved);

    // 应用缩放：字号随 Ctrl+滚轮缩放（图片宽度在第四步同步缩放）
    QFont docFont = document()->defaultFont();
    if(m_nBaseFontSize <= 0) {
        // 首次渲染时记录基准字号（pixelSize 无效时按 pointSize 换算）
        m_nBaseFontSize = docFont.pixelSize() > 0
            ? docFont.pixelSize()
            : qMax(8, qRound(docFont.pointSizeF() * 1.333));
    }
    docFont.setPixelSize(qMax(6, qRound(m_nBaseFontSize * m_dZoom)));
    document()->setDefaultFont(docFont);

    // 第三步：注册页面引用图片 resource
    for(const auto& rRef : vecPageRefs) {
        const QImage image = renderPageRef(rRef.first, rRef.second);
        if(!image.isNull()) {
            const QUrl resourceUrl(QStringLiteral("bwm://page/%1/%2")
                                       .arg(rRef.first).arg(rRef.second));
            document()->addResource(QTextDocument::ImageResource, resourceUrl, image);
        }
    }

    // 第四步：遍历 document 中所有图片，加载并注册普通图片 resource，
    //         同时按缩放因子设置显示宽度（与文字同步缩放）。
    const int nMaxImageWidth = qMax(50, qRound((viewport()->width() - 40) * m_dZoom));
    for(QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        for(QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
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

            // 跳过已注册的页面引用图片（bwm:// 协议）
            if(url.scheme() != QStringLiteral("bwm")) {
                // 加载普通图片
                QString strPath = url.path();
                if(strPath.isEmpty()) {
                    strPath = strName;
                }
                QFileInfo info(strPath);
                if(info.isRelative() && !m_strProjectDirectory.isEmpty()) {
                    strPath = QDir(m_strProjectDirectory).filePath(strPath);
                }
                QFile file(strPath);
                if(file.open(QIODevice::ReadOnly)) {
                    QImage image;
                    if(image.loadFromData(file.readAll())) {
                        document()->addResource(QTextDocument::ImageResource, url, image);
                    }
                }
            }

            // 限制图片最大宽度
            if(nMaxImageWidth > 100) {
                const int nCurrentWidth = imageFormat.width();
                if(nCurrentWidth <= 0 || nCurrentWidth > nMaxImageWidth) {
                    QTextCursor cursor(block);
                    cursor.setPosition(fragment.position());
                    cursor.setPosition(fragment.position() + fragment.length(),
                                        QTextCursor::KeepAnchor);
                    imageFormat.setWidth(nMaxImageWidth);
                    cursor.setCharFormat(imageFormat);
                }
            }
        }
    }
}

QVariant MarkdownPreview::loadResource(int nType, const QUrl& rName)
{
    return QTextBrowser::loadResource(nType, rName);
}

void MarkdownPreview::wheelEvent(QWheelEvent* pEvent)
{
    // Ctrl+滚轮：整体缩放（文字与图片同步），并保持相对滚动位置
    if(pEvent->modifiers() & Qt::ControlModifier) {
        const int nDelta = pEvent->angleDelta().y();
        if(nDelta != 0) {
            const qreal dOldZoom = m_dZoom;
            m_dZoom = qBound(0.3, m_dZoom * (nDelta > 0 ? 1.1 : 1.0 / 1.1), 5.0);
            if(!qFuzzyCompare(dOldZoom, m_dZoom)) {
                QScrollBar* pScrollBar = verticalScrollBar();
                const int nOldMax = pScrollBar->maximum();
                const qreal dRatio = nOldMax > 0
                    ? qreal(pScrollBar->value()) / nOldMax : 0.0;
                renderContent();
                pScrollBar->setValue(qRound(dRatio * pScrollBar->maximum()));
            }
        }
        pEvent->accept();
        return;
    }
    QTextBrowser::wheelEvent(pEvent);
}

QImage MarkdownPreview::renderPageRef(int nWalkthroughIndex, int nPageIndex)
{
    if(!m_pProject
       || nWalkthroughIndex < 0
       || nWalkthroughIndex >= m_pProject->vecWalkthroughs.size()) {
        return QImage();
    }
    const Walkthrough& rWalkthrough = m_pProject->vecWalkthroughs.at(nWalkthroughIndex);
    if(nPageIndex < 0 || nPageIndex >= rWalkthrough.vecPages.size()) {
        return QImage();
    }
    // 用 0.5 倍率渲染，避免预览中页面图片过大
    return ExportRenderer::renderPage(rWalkthrough.vecPages.at(nPageIndex),
                                      0.5, m_backgroundColor, QString(),
                                      m_strProjectDirectory);
}

} // namespace bwm
