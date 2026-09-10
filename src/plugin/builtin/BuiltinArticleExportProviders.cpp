/**
 * @file BuiltinArticleExportProviders.cpp
 * @author zhangweimu
 * @brief 内置文章导出格式 Provider 实现。
 */
#include "plugin/builtin/BuiltinArticleExportProviders.h"

#include "core/Article.h"
#include "core/Project.h"
#include "export/ArticleRenderer.h"
#include "export/ExportRenderer.h"
#include "export/ExportResultHelper.h"
#include "plugin/PluginContext.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QPrinter>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTextCursor>

namespace bwm {

// =========================================================================
// ArticleMarkdownExportProvider
// =========================================================================

QString ArticleMarkdownExportProvider::formatId() const
{
    return QStringLiteral("article.markdown");
}

QString ArticleMarkdownExportProvider::displayName() const
{
    return QStringLiteral("Markdown 文件（.md + 图片）");
}

bool ArticleMarkdownExportProvider::supportsArticle() const
{
    return true;
}

int ArticleMarkdownExportProvider::exportArticle(const Article& rArticle,
                                                   const Project& rProject,
                                                   const QString& rArticleTitle,
                                                   const QString& strDirPath,
                                                   const QString& strAuthor,
                                                   const PluginContext& rContext,
                                                   QWidget* pParent) const
{
    (void)strAuthor;   // Markdown 导出保持原文纯净，不追加署名
    const QString strSafeName = ArticleRenderer::sanitizeFileName(rArticleTitle);
    if(strSafeName.isEmpty()) {
        return 0;
    }

    QDir exportDir(strDirPath);
    if(!exportDir.exists()) {
        exportDir.mkpath(QStringLiteral("."));
    }

    // images/ 子目录存放页面引用渲染图和复制的素材图片
    const QString strImagesDir = exportDir.filePath(QStringLiteral("images"));
    QDir imagesDir(strImagesDir);
    if(!imagesDir.exists()) {
        imagesDir.mkpath(QStringLiteral("."));
    }

    // 1. 主文件：原样输出 Markdown（保留 ![[W:P]]）
    const QString strMainPath = exportDir.filePath(strSafeName + QStringLiteral(".md"));
    QFile mainFile(strMainPath);
    if(!mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return 0;
    }
    mainFile.write(rArticle.strMarkdown.toUtf8());
    mainFile.close();

    // 2. 导出页面引用渲染图 + 构造兼容版 Markdown
    QString strCompatible = rArticle.strMarkdown;
    const QRegularExpression refRegex(kPageRefPattern);
    QRegularExpressionMatchIterator it = refRegex.globalMatch(rArticle.strMarkdown);
    QVector<QRegularExpressionMatch> vecMatches;
    while(it.hasNext()) {
        vecMatches.append(it.next());
    }
    // 倒序替换
    for(int i = vecMatches.size() - 1; i >= 0; --i) {
        const QRegularExpressionMatch& rMatch = vecMatches.at(i);
        const int nW = rMatch.captured(1).toInt();
        const int nP = rMatch.captured(2).toInt();
        // 渲染页面为图片
        const QImage image = ArticleRenderer::renderPageRef(
            rProject, nW, nP, rContext.theme.backgroundColor, 1.0,
            rContext.projectDirectory);
        if(image.isNull()) {
            continue;
        }
        const QString strImageName = QStringLiteral("page_%1_%2.png").arg(nW).arg(nP);
        const QString strImagePath = imagesDir.filePath(strImageName);
        if(!ExportRenderer::writePng(image, strImagePath, nullptr)) {
            continue;
        }
        // 兼容版替换为标准图片语法
        const QString strReplacement = QStringLiteral("![页面%1:%2](images/%3)")
                                             .arg(nW).arg(nP).arg(strImageName);
        strCompatible.replace(rMatch.capturedStart(), rMatch.capturedLength(),
                              strReplacement);
    }

    // 3. 复制普通素材图片到 images/ 并改写路径
    const QRegularExpression imgRegex(QStringLiteral(R"(!\[([^\]]*)\]\(([^)]+)\))"));
    QRegularExpressionMatchIterator imgIt = imgRegex.globalMatch(strCompatible);
    QVector<QRegularExpressionMatch> vecImgMatches;
    while(imgIt.hasNext()) {
        vecImgMatches.append(imgIt.next());
    }
    for(int i = vecImgMatches.size() - 1; i >= 0; --i) {
        const QRegularExpressionMatch& rMatch = vecImgMatches.at(i);
        QString strUrl = rMatch.captured(2);
        if(strUrl.startsWith(QStringLiteral("images/"))) {
            continue;   // 已是 images/ 路径，跳过
        }
        // 解析源文件路径
        QString strSrcPath = strUrl;
        QFileInfo info(strSrcPath);
        if(info.isRelative() && !rContext.projectDirectory.isEmpty()) {
            strSrcPath = QDir(rContext.projectDirectory).filePath(strSrcPath);
        }
        if(!QFileInfo(strSrcPath).exists()) {
            continue;
        }
        // 复制到 images/
        const QString strDestName = QFileInfo(strSrcPath).fileName();
        const QString strDestPath = imagesDir.filePath(strDestName);
        QFile::remove(strDestPath);
        if(!QFile::copy(strSrcPath, strDestPath)) {
            continue;
        }
        // 改写路径为 images/xxx
        QString strNewMarkdown = rMatch.captured(0);
        strNewMarkdown.replace(strUrl, QStringLiteral("images/") + strDestName);
        strCompatible.replace(rMatch.capturedStart(), rMatch.capturedLength(), strNewMarkdown);
    }

    // 4. 兼容版 .md
    const QString strCompatPath = exportDir.filePath(
        strSafeName + QStringLiteral("_compatible.md"));
    QFile compatFile(strCompatPath);
    if(compatFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        compatFile.write(strCompatible.toUtf8());
        compatFile.close();
    }

    // 导出完成提示（3 个文件：主 .md + 兼容版 .md + images/ 目录）
    showExportResult(pParent, 1, strDirPath, QStringLiteral("篇文章（含 images 目录）"));
    return 1;   // 主文件导出成功
}

// =========================================================================
// ArticlePngExportProvider
// =========================================================================

QString ArticlePngExportProvider::formatId() const
{
    return QStringLiteral("article.png.longimage");
}

QString ArticlePngExportProvider::displayName() const
{
    return QStringLiteral("PNG 长图（整篇文章渲染为一张图）");
}

bool ArticlePngExportProvider::supportsArticle() const
{
    return true;
}

int ArticlePngExportProvider::exportArticle(const Article& rArticle,
                                              const Project& rProject,
                                              const QString& rArticleTitle,
                                              const QString& strDirPath,
                                              const QString& strAuthor,
                                              const PluginContext& rContext,
                                              QWidget* pParent) const
{
    const int nImageWidth = 1080;   // 长图宽度
    QScopedPointer<QTextDocument> pDocument(
        ArticleRenderer::buildDocument(rArticle, rProject, rContext, nImageWidth));

    // 计算文档总高度：设置文本宽度后由 documentLayout 给出实际尺寸
    pDocument->setTextWidth(nImageWidth);
    const QSizeF docSize = pDocument->documentLayout()->documentSize();
    qreal dHeight = docSize.height();
    if(dHeight < 100) {
        dHeight = 100;   // 防止空文档
    }

    // 高度上限检测（与图文长图一致）：QImage 尺寸受限（约 32767px），
    // 超出时明确提示而非静默失败。
    constexpr int nMaxLongImageHeight = 30000;
    if(dHeight > nMaxLongImageHeight) {
        QMessageBox::warning(pParent, QStringLiteral("导出 PNG 长图"),
                             QStringLiteral("文章过长（%1px，上限 %2px），请拆分文章后导出")
                                 .arg(int(dHeight)).arg(nMaxLongImageHeight));
        return 0;
    }

    // 渲染到 QImage
    QImage image(nImageWidth, int(dHeight) + 20, QImage::Format_ARGB32);
    image.fill(rContext.theme.backgroundColor);
    QPainter painter(&image);
    pDocument->drawContents(&painter);
    painter.end();

    // 作者署名水印（右下角，半透明）；署名大小随图片宽度缩放
    ExportRenderer::drawAuthorMark(image, strAuthor, image.width() / 720.0);

    // 写 PNG
    const QString strSafeName = ArticleRenderer::sanitizeFileName(rArticleTitle);
    const QString strFilePath = QDir(strDirPath).filePath(
        strSafeName + QStringLiteral(".png"));
    QString strError;
    if(!ExportRenderer::writePng(image, strFilePath, &strError)) {
        return 0;
    }
    showExportResult(pParent, 1, strDirPath, QStringLiteral("张长图"));
    return 1;
}

// =========================================================================
// ArticlePdfExportProvider
// =========================================================================

QString ArticlePdfExportProvider::formatId() const
{
    return QStringLiteral("article.pdf");
}

QString ArticlePdfExportProvider::displayName() const
{
    return QStringLiteral("PDF 文档（分页）");
}

bool ArticlePdfExportProvider::supportsArticle() const
{
    return true;
}

int ArticlePdfExportProvider::exportArticle(const Article& rArticle,
                                              const Project& rProject,
                                              const QString& rArticleTitle,
                                              const QString& strDirPath,
                                              const QString& strAuthor,
                                              const PluginContext& rContext,
                                              QWidget* pParent) const
{
    const int nImageWidth = 800;   // PDF 页面较窄，适合阅读
    QScopedPointer<QTextDocument> pDocument(
        ArticleRenderer::buildDocument(rArticle, rProject, rContext, nImageWidth));

    // 作者署名：在文档末尾追加一行（字号约为正文的 3/4，颜色跟随主题）
    if(!strAuthor.trimmed().isEmpty()) {
        QTextCursor cursor(pDocument.data());
        cursor.movePosition(QTextCursor::End);
        cursor.insertBlock();
        QTextCharFormat authorFormat;
        authorFormat.setForeground(rContext.theme.textColor);
        QFont authorFont = pDocument->defaultFont();
        authorFont.setPixelSize(qMax(12, authorFont.pixelSize() * 3 / 4));
        authorFormat.setFont(authorFont);
        cursor.insertText(QStringLiteral("— by %1").arg(strAuthor), authorFormat);
    }

    const QString strSafeName = ArticleRenderer::sanitizeFileName(rArticleTitle);
    const QString strFilePath = QDir(strDirPath).filePath(
        strSafeName + QStringLiteral(".pdf"));

    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(strFilePath);
    printer.setPageSize(QPageSize(QPageSize::A4));
    pDocument->print(&printer);

    if(!QFileInfo(strFilePath).exists()) {
        return 0;
    }
    showExportResult(pParent, 1, strDirPath, QStringLiteral("个 PDF 文档"));
    return 1;
}

} // namespace bwm
