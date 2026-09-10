/**
 * @file test_export_renderer.cpp
 * @author zhangweimu
 * @brief PNG 导出渲染的单元测试（单页尺寸 / 长图拼接 / 像素内容 / 文件写入）。
 */
#include <QtTest>

#include <QDir>
#include <QImage>
#include <QTemporaryDir>

#include "core/Project.h"
#include "export/ExportRenderer.h"

using namespace bwm;

class TestExportRenderer : public QObject {
    Q_OBJECT

private slots:
    // 单页渲染尺寸 = 页面尺寸 × 倍率
    void testRenderPageSize();
    // 渲染结果包含组件内容（非纯白）
    void testRenderPageContainsContent();
    // 长图拼接：高度 = 各页高度之和（含分隔线），宽度取最大页宽
    void testRenderLongImageSize();
    // 空页面列表返回空图
    void testRenderLongImageEmpty();
    // PNG 写入文件成功
    void testWritePng();
    // 页面背景图：传入项目目录时铺满整页，未传时保留背景色；长图同样生效
    void testRenderPageWithBackgroundImage();
};

// 构造一个含形状与文本组件的页面
Page makeSamplePage()
{
    Page page;
    page.strName = QStringLiteral("页面 1");
    page.size = QSize(200, 100);

    Component shape;
    shape.strId = QStringLiteral("shape-1");
    shape.eType = E_COMPONENT_TYPE_SHAPE;
    shape.pos = QPointF(10, 10);
    shape.size = QSizeF(80, 60);
    shape.shapeData.fillColor = QColor(255, 0, 0);
    shape.shapeData.borderColor = QColor(Qt::black);
    page.vecComponents.append(shape);

    Component text;
    text.strId = QStringLiteral("text-1");
    text.eType = E_COMPONENT_TYPE_TEXT;
    text.pos = QPointF(100, 20);
    text.size = QSizeF(90, 30);
    text.textData.strContent = QStringLiteral("测试文本");
    text.textData.nFontSize = 14;
    page.vecComponents.append(text);
    return page;
}

void TestExportRenderer::testRenderPageSize()
{
    const Page page = makeSamplePage();
    const QImage image = ExportRenderer::renderPage(page, 2.0);
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 400);
    QCOMPARE(image.height(), 200);
}

void TestExportRenderer::testRenderPageContainsContent()
{
    const Page page = makeSamplePage();
    const QImage image = ExportRenderer::renderPage(page, 1.0);
    // 左上角红色形状区域应为非白像素（原图白色背景，形状填充红色）
    bool bFoundRed = false;
    for (int nX = 10; nX < 90 && !bFoundRed; ++nX) {
        for (int nY = 10; nY < 70; ++nY) {
            if (image.pixelColor(nX, nY).red() > 200) {
                bFoundRed = true;
                break;
            }
        }
    }
    QVERIFY(bFoundRed);
}

void TestExportRenderer::testRenderLongImageSize()
{
    Page pageA = makeSamplePage();
    Page pageB = makeSamplePage();
    pageB.size = QSize(180, 80);
    const QVector<Page> pages = {pageA, pageB};

    // 无分隔线：高 = 100 + 80，宽 = max(200, 180) = 200
    QString strErrorMessage;
    QImage image = ExportRenderer::renderLongImage(pages, 1.0, false, &strErrorMessage);
    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 200);
    QCOMPARE(image.height(), 180);

    // 有分隔线：高 = 100 + 20 + 80 = 200
    image = ExportRenderer::renderLongImage(pages, 1.0, true, &strErrorMessage);
    QVERIFY(!image.isNull());
    QCOMPARE(image.height(), 200);
}

void TestExportRenderer::testRenderLongImageEmpty()
{
    QString strErrorMessage;
    QVERIFY(ExportRenderer::renderLongImage(QVector<Page>(), 1.0, false, &strErrorMessage).isNull());
}

void TestExportRenderer::testWritePng()
{
    QTemporaryDir tempDir;
    const QString strFilePath = tempDir.filePath(QStringLiteral("导出.png"));
    const Page page = makeSamplePage();
    const QImage image = ExportRenderer::renderPage(page, 1.0);
    QString strErrorMessage;
    QVERIFY2(ExportRenderer::writePng(image, strFilePath, &strErrorMessage), qPrintable(strErrorMessage));
    QVERIFY(QFile::exists(strFilePath));

    // 写入失败路径（目录不存在）
    QVERIFY(!ExportRenderer::writePng(image, tempDir.path() + QStringLiteral("/不存在/xx.png"),
                                      &strErrorMessage));
}

void TestExportRenderer::testRenderPageWithBackgroundImage()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    // 准备项目内素材：assets/bg.png（纯蓝 40x20）
    QVERIFY(QDir(tempDir.path()).mkpath(QStringLiteral("assets")));
    QImage background(40, 20, QImage::Format_ARGB32);
    background.fill(QColor(0, 0, 255));
    QVERIFY(background.save(tempDir.filePath(QStringLiteral("assets/bg.png")), "PNG"));

    Page page = makeSamplePage();
    page.size = QSize(40, 20);
    page.vecComponents.clear();   // 只验证背景图绘制
    page.strBackgroundImage = QStringLiteral("assets/bg.png");

    // 传入项目目录：相对路径被解析，背景图铺满整页（页面尺寸 = 图片尺寸）
    const QImage image = ExportRenderer::renderPage(page, 1.0, Qt::white, QString(), tempDir.path());
    QCOMPARE(image.size(), QSize(40, 20));
    QCOMPARE(image.pixelColor(5, 5), QColor(0, 0, 255));

    // 不传项目目录：相对路径无法解析，保留下主题背景色（不崩溃、不绘制）
    const QImage noDirImage = ExportRenderer::renderPage(page, 1.0, Qt::white);
    QCOMPARE(noDirImage.pixelColor(5, 5), QColor(Qt::white));

    // 长图渲染同样带上背景图
    QString strErrorMessage;
    const QImage longImage = ExportRenderer::renderLongImage({page}, 1.0, false, &strErrorMessage,
                                                             Qt::white, QString(), tempDir.path());
    QVERIFY(!longImage.isNull());
    QCOMPARE(longImage.pixelColor(5, 5), QColor(0, 0, 255));
}

// 文本渲染需要 QGuiApplication（字体数据库），因此用 QTEST_MAIN
QTEST_MAIN(TestExportRenderer)

#include "test_export_renderer.moc"
