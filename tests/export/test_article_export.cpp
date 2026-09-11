/**
 * @file test_article_export.cpp
 * @author zhangweimu
 * @brief 文章导入导出功能的单元测试。
 *
 * 覆盖：
 * - ArticleImporter 基本导入（读取 .md，标题取文件名）
 * - ArticleImporter 复制本地图片到 assets/ 并改写路径
 * - ArticleRenderer::buildDocument 构造文档
 * - 三种导出 Provider：Markdown / PNG / PDF
 */
#include <QtTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QScopedPointer>
#include <QTemporaryDir>
#include <QTextDocument>

#include "core/Article.h"
#include "core/Project.h"
#include "export/ArticleImporter.h"
#include "export/ArticleRenderer.h"
#include "plugin/PluginContext.h"
#include "plugin/builtin/BuiltinArticleExportProviders.h"

using namespace bwm;

class TestArticleExport : public QObject {
    Q_OBJECT

private slots:
    void testImportBasic();
    void testImportCopiesImages();
    void testBuildDocument();
    void testMarkdownExport();
    void testPngExport();
    void testPngAuthorMark();
    // 署名样式（位置/字号/颜色）经 PluginContext 传给 Provider 并生效
    void testPngAuthorMarkStyle();
    void testPngTextVisibleInDarkTheme();
    void testPdfExport();
    void testSanitizeFileName();

private:
    // 构造测试用项目：一个攻略含一个页面
    static Project makeProject();
    // 构造测试用文章
    static Article makeArticle();
    // 构造测试用上下文
    static PluginContext makeContext(const QString& strProjectDir);
    // 写一个测试用 .md 文件，返回路径
    static QString writeTestMarkdown(const QString& strDir, const QString& strName,
                                     const QString& strContent);
    // 写一个测试用 PNG 图片，返回路径
    static QString writeTestImage(const QString& strDir, const QString& strName);
};

Project TestArticleExport::makeProject()
{
    Project project;
    project.strName = QStringLiteral("测试游戏");

    Walkthrough walkthrough;
    walkthrough.strTitle = QStringLiteral("测试攻略");
    walkthrough.eType = E_WALKTHROUGH_TYPE_EQUIPMENT;

    Page page;
    page.strName = QStringLiteral("测试页面");
    page.size = QSize(1080, 1440);
    walkthrough.vecPages.append(page);

    project.vecWalkthroughs.append(walkthrough);
    return project;
}

Article TestArticleExport::makeArticle()
{
    Article article;
    article.strId = QStringLiteral("test-article");
    article.strTitle = QStringLiteral("测试文章");
    article.strMarkdown = QStringLiteral(
        "# 测试标题\n\n"
        "这是一段正文。\n\n"
        "## 二级标题\n\n"
        "- 列表项 1\n"
        "- 列表项 2\n");
    return article;
}

PluginContext TestArticleExport::makeContext(const QString& strProjectDir)
{
    PluginContext ctx;
    ctx.theme = ThemeManager::currentTheme();
    ctx.defaultPageSize = QSize(1080, 1440);
    ctx.projectDirectory = strProjectDir;
    return ctx;
}

QString TestArticleExport::writeTestMarkdown(const QString& strDir, const QString& strName,
                                              const QString& strContent)
{
    const QString strPath = QDir(strDir).filePath(strName);
    QFile file(strPath);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(strContent.toUtf8());
        file.close();
    }
    return strPath;
}

QString TestArticleExport::writeTestImage(const QString& strDir, const QString& strName)
{
    const QString strPath = QDir(strDir).filePath(strName);
    QImage image(100, 80, QImage::Format_ARGB32);
    image.fill(Qt::red);
    image.save(strPath, "PNG");
    return strPath;
}

void TestArticleExport::testImportBasic()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString strContent = QStringLiteral("# 导入测试\n\n正文内容\n");
    const QString strMdPath = writeTestMarkdown(tempDir.path(),
                                                 QStringLiteral("我的攻略.md"),
                                                 strContent);

    // 项目目录：临时目录下的 project 子目录
    const QString strProjectDir = QDir(tempDir.path()).filePath(QStringLiteral("project"));
    QDir().mkpath(strProjectDir);

    QString strError;
    const Article article = ArticleImporter::importFromFile(strMdPath, strProjectDir, &strError);

    QVERIFY(strError.isEmpty());
    QCOMPARE(article.strTitle, QStringLiteral("我的攻略"));
    QVERIFY(article.strMarkdown.contains(QStringLiteral("导入测试")));
    QVERIFY(article.strMarkdown.contains(QStringLiteral("正文内容")));
    QVERIFY(!article.strId.isEmpty());
}

void TestArticleExport::testImportCopiesImages()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // 创建一张图片和引用它的 .md
    writeTestImage(tempDir.path(), QStringLiteral("screenshot.png"));
    const QString strContent = QStringLiteral(
        "# 含图片的文章\n\n"
        "![截图](screenshot.png)\n\n"
        "正文\n");
    const QString strMdPath = writeTestMarkdown(tempDir.path(),
                                                 QStringLiteral("图片文章.md"),
                                                 strContent);

    const QString strProjectDir = QDir(tempDir.path()).filePath(QStringLiteral("project"));
    QDir().mkpath(strProjectDir);

    QString strError;
    const Article article = ArticleImporter::importFromFile(strMdPath, strProjectDir, &strError);

    QVERIFY(strError.isEmpty());
    // 图片应被复制到 assets/ 且路径被改写
    QVERIFY(article.strMarkdown.contains(QStringLiteral("assets/")));
    QVERIFY(!article.strMarkdown.contains(QStringLiteral("(screenshot.png)")));

    // 验证 assets 目录下确实有文件
    QDir assetsDir(QDir(strProjectDir).filePath(QStringLiteral("assets")));
    QVERIFY(assetsDir.exists());
    const QStringList files = assetsDir.entryList(QDir::Files);
    QCOMPARE(files.size(), 1);
}

void TestArticleExport::testBuildDocument()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const Project project = makeProject();
    const Article article = makeArticle();
    const PluginContext ctx = makeContext(tempDir.path());

    QScopedPointer<QTextDocument> pDocument(
        ArticleRenderer::buildDocument(article, project, ctx, 800));

    QVERIFY(!pDocument.isNull());
    // 文档应包含渲染后的文本内容
    const QString strPlainText = pDocument->toPlainText();
    QVERIFY(strPlainText.contains(QStringLiteral("测试标题")));
    QVERIFY(strPlainText.contains(QStringLiteral("这是一段正文")));
    QVERIFY(strPlainText.contains(QStringLiteral("列表项 1")));
}

void TestArticleExport::testMarkdownExport()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const Project project = makeProject();
    const Article article = makeArticle();
    const PluginContext ctx = makeContext(tempDir.path());
    const QString strExportDir = QDir(tempDir.path()).filePath(QStringLiteral("export"));
    QDir().mkpath(strExportDir);

    ArticleMarkdownExportProvider provider;
    QVERIFY(provider.supportsArticle());

    const int nExported = provider.exportArticle(
        article, project, article.strTitle, strExportDir, QString(), ctx, nullptr);

    QCOMPARE(nExported, 1);

    // 主文件应存在
    const QString strMainPath = QDir(strExportDir).filePath(QStringLiteral("测试文章.md"));
    QVERIFY2(QFile::exists(strMainPath), qPrintable(strMainPath));

    // 兼容版应存在
    const QString strCompatPath = QDir(strExportDir).filePath(QStringLiteral("测试文章_compatible.md"));
    QVERIFY(QFile::exists(strCompatPath));

    // 主文件内容应保留原文
    QFile mainFile(strMainPath);
    QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString strMainContent = QString::fromUtf8(mainFile.readAll());
    mainFile.close();
    QVERIFY(strMainContent.contains(QStringLiteral("测试标题")));
}

void TestArticleExport::testPngExport()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const Project project = makeProject();
    const Article article = makeArticle();
    const PluginContext ctx = makeContext(tempDir.path());
    const QString strExportDir = QDir(tempDir.path()).filePath(QStringLiteral("export"));
    QDir().mkpath(strExportDir);

    ArticlePngExportProvider provider;
    QVERIFY(provider.supportsArticle());

    const int nExported = provider.exportArticle(
        article, project, article.strTitle, strExportDir, QString(), ctx, nullptr);

    QCOMPARE(nExported, 1);

    const QString strPngPath = QDir(strExportDir).filePath(QStringLiteral("测试文章.png"));
    QVERIFY2(QFile::exists(strPngPath), qPrintable(strPngPath));

    // 验证是有效的 PNG 且尺寸合理
    QImage image(strPngPath);
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 1080);   // 长图固定宽度
    QVERIFY(image.height() > 50);
}

void TestArticleExport::testPngAuthorMark()
{
    // 指定作者时，PNG 右下角应绘制署名水印（存在非背景色像素）；
    // 不指定作者时，右下角应与背景一致。
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const Project project = makeProject();
    const Article article = makeArticle();

    PluginContext ctx = makeContext(tempDir.path());
    ctx.theme.backgroundColor = QColor(30, 30, 46);
    ctx.theme.textColor = QColor(237, 237, 237);

    // 统计图片右下角区域的非背景像素数
    auto countMarkPixels = [](const QImage& rImage, const QColor& rBackground) {
        const QRect markArea(rImage.width() * 3 / 5, rImage.height() * 4 / 5,
                             rImage.width() * 2 / 5, rImage.height() / 5);
        int nCount = 0;
        for(int y = markArea.top(); y < markArea.bottom() && y < rImage.height(); ++y) {
            for(int x = markArea.left(); x < markArea.right() && x < rImage.width(); ++x) {
                if(rImage.pixel(x, y) != rBackground.rgb()) {
                    ++nCount;
                }
            }
        }
        return nCount;
    };

    // ---- 无署名 ----
    const QString strDirNoAuthor = QDir(tempDir.path()).filePath(QStringLiteral("no_author"));
    QDir().mkpath(strDirNoAuthor);
    ArticlePngExportProvider provider;
    QCOMPARE(provider.exportArticle(article, project, QStringLiteral("无署名"),
                                    strDirNoAuthor, QString(), ctx, nullptr), 1);
    const QImage imageNoAuthor(QDir(strDirNoAuthor).filePath(QStringLiteral("无署名.png")));
    QVERIFY(!imageNoAuthor.isNull());
    const int nPixelsNoAuthor = countMarkPixels(imageNoAuthor, ctx.theme.backgroundColor);

    // ---- 有署名 ----
    const QString strDirWithAuthor = QDir(tempDir.path()).filePath(QStringLiteral("with_author"));
    QDir().mkpath(strDirWithAuthor);
    QCOMPARE(provider.exportArticle(article, project, QStringLiteral("有署名"),
                                    strDirWithAuthor, QStringLiteral("测试作者"), ctx, nullptr), 1);
    const QImage imageWithAuthor(QDir(strDirWithAuthor).filePath(QStringLiteral("有署名.png")));
    QVERIFY(!imageWithAuthor.isNull());
    const int nPixelsWithAuthor = countMarkPixels(imageWithAuthor, ctx.theme.backgroundColor);

    // 有署名的右下角区域应有更多非背景像素
    QVERIFY2(nPixelsWithAuthor > nPixelsNoAuthor,
             qPrintable(QStringLiteral("署名未绘制：无署名 %1 像素，有署名 %2 像素")
                            .arg(nPixelsNoAuthor).arg(nPixelsWithAuthor)));
}

void TestArticleExport::testPngAuthorMarkStyle()
{
    // 署名样式经 PluginContext 传到 Provider：左上角 + 大字号 + 红色应生效。
    // 校验方式：与「无署名」导出图逐像素比较，差异只允许落在左上角区域。
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const Project project = makeProject();
    const Article article = makeArticle();
    ArticlePngExportProvider provider;

    // ---- 基准图：无署名 ----
    const QString strDirNoAuthor = QDir(tempDir.path()).filePath(QStringLiteral("style_none"));
    QDir().mkpath(strDirNoAuthor);
    QCOMPARE(provider.exportArticle(article, project, QStringLiteral("无署名"),
                                    strDirNoAuthor, QString(), makeContext(tempDir.path()),
                                    nullptr), 1);
    const QImage imageNoAuthor(QDir(strDirNoAuthor).filePath(QStringLiteral("无署名.png")));
    QVERIFY(!imageNoAuthor.isNull());

    // ---- 自定义样式：左上角 / 48px / 红色 / 不透明 ----
    PluginContext ctx = makeContext(tempDir.path());
    ctx.authorMarkStyle.ePosition = E_AUTHOR_MARK_POSITION_TOP_LEFT;
    ctx.authorMarkStyle.nFontSize = 48;
    ctx.authorMarkStyle.color = QColor(Qt::red);
    ctx.authorMarkStyle.nOpacityPercent = 100;

    const QString strDirStyled = QDir(tempDir.path()).filePath(QStringLiteral("style_topleft"));
    QDir().mkpath(strDirStyled);
    QCOMPARE(provider.exportArticle(article, project, QStringLiteral("左上署名"),
                                    strDirStyled, QStringLiteral("测试作者"), ctx, nullptr), 1);
    const QImage imageStyled(QDir(strDirStyled).filePath(QStringLiteral("左上署名.png")));
    QVERIFY(!imageStyled.isNull());
    QCOMPARE(imageStyled.size(), imageNoAuthor.size());

    // 差异统计：左上区域应有差异，其余区域必须完全一致（说明水印没画到别处）
    int nDiffTopLeft = 0;
    int nDiffOther = 0;
    for(int nY = 0; nY < imageNoAuthor.height(); ++nY) {
        for(int nX = 0; nX < imageNoAuthor.width(); ++nX) {
            if(imageNoAuthor.pixel(nX, nY) == imageStyled.pixel(nX, nY)) {
                continue;
            }
            if(nX < imageNoAuthor.width() / 2 && nY < imageNoAuthor.height() / 2) {
                ++nDiffTopLeft;
            } else {
                ++nDiffOther;
            }
        }
    }
    QVERIFY2(nDiffTopLeft > 0, "左上角署名样式未生效");
    QCOMPARE(nDiffOther, 0);
}

void TestArticleExport::testPngTextVisibleInDarkTheme()
{
    // 深色主题下文字必须可见（背景深蓝灰 + 近白文字），
    // 验证 PNG 中同时存在背景色像素与文字色像素。
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const Project project = makeProject();
    const Article article = makeArticle();

    PluginContext ctx = makeContext(tempDir.path());
    ctx.theme.backgroundColor = QColor(30, 30, 46);    // 深色游戏风背景
    ctx.theme.textColor = QColor(237, 237, 237);       // 深色游戏风文字

    const QString strExportDir = QDir(tempDir.path()).filePath(QStringLiteral("export"));
    QDir().mkpath(strExportDir);

    ArticlePngExportProvider provider;
    QCOMPARE(provider.exportArticle(article, project, article.strTitle,
                                    strExportDir, QString(), ctx, nullptr), 1);

    const QString strPngPath = QDir(strExportDir).filePath(QStringLiteral("测试文章.png"));
    QVERIFY(QFile::exists(strPngPath));

    const QImage image(strPngPath);
    QVERIFY(!image.isNull());

    // 统计背景色与非背景色像素
    const QRgb backgroundRgb = ctx.theme.backgroundColor.rgb();
    int nBackgroundPixels = 0;
    int nOtherPixels = 0;
    for(int y = 0; y < image.height(); y += 2) {
        for(int x = 0; x < image.width(); x += 2) {
            if(image.pixel(x, y) == backgroundRgb) {
                ++nBackgroundPixels;
            } else {
                ++nOtherPixels;
            }
        }
    }
    // 应同时存在背景像素与文字（非背景）像素
    QVERIFY2(nBackgroundPixels > 0, "背景像素缺失");
    QVERIFY2(nOtherPixels > 0, "文字像素缺失（深色主题下文字可能不可见）");
}

void TestArticleExport::testPdfExport()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const Project project = makeProject();
    const Article article = makeArticle();
    const PluginContext ctx = makeContext(tempDir.path());
    const QString strExportDir = QDir(tempDir.path()).filePath(QStringLiteral("export"));
    QDir().mkpath(strExportDir);

    ArticlePdfExportProvider provider;
    QVERIFY(provider.supportsArticle());

    // 传署名，验证 PDF 署名路径不崩溃
    const int nExported = provider.exportArticle(
        article, project, article.strTitle, strExportDir, QStringLiteral("测试作者"),
        ctx, nullptr);

    QCOMPARE(nExported, 1);

    const QString strPdfPath = QDir(strExportDir).filePath(QStringLiteral("测试文章.pdf"));
    QVERIFY2(QFile::exists(strPdfPath), qPrintable(strPdfPath));

    // PDF 文件应非空
    QVERIFY(QFileInfo(strPdfPath).size() > 0);
}

void TestArticleExport::testSanitizeFileName()
{
    QCOMPARE(ArticleRenderer::sanitizeFileName(QStringLiteral("正常名称")),
             QStringLiteral("正常名称"));
    // 非法字符替换为下划线
    const QString strResult = ArticleRenderer::sanitizeFileName(
        QStringLiteral("含/非法\\字符:测试"));
    QVERIFY(!strResult.contains(QLatin1Char('/')));
    QVERIFY(!strResult.contains(QLatin1Char('\\')));
    QVERIFY(!strResult.contains(QLatin1Char(':')));
}

// 文章渲染需要 QGuiApplication（字体数据库 + QPrinter），因此用 QTEST_MAIN
QTEST_MAIN(TestArticleExport)

#include "test_article_export.moc"
