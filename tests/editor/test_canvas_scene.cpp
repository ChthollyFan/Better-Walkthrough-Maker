/**
 * @file test_canvas_scene.cpp
 * @author zhangweimu
 * @brief 画布场景的单元测试（页面背景图 / 主题背景色 / 组件的层序）。
 *
 * 回归背景：页面背景图图元与主题背景色矩形的 Z 值顺序曾写反
 * （图片 -2、不透明背景矩形 -1），矩形把整张背景图盖住，导致
 * 「从图片新建页面」「设置背景图」后画布上永远是一片白。
 * 导出渲染（先填色再画图）不受影响，所以只有画布表现异常。
 * 这里通过「把场景渲染成图片后取像素」锁定层序，防止再次写反。
 */
#include <QtTest>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>

#include "core/Project.h"
#include "editor/CanvasScene.h"
#include "editor/ComponentItem.h"

using namespace bwm;

class TestCanvasScene : public QObject {
    Q_OBJECT

private slots:
    // 有背景图时页面区域显示背景图，而不是被主题背景色盖住
    void testBackgroundImageCoversPageColor();
    // 无背景图时显示主题背景色
    void testPageColorWithoutBackgroundImage();
    // 背景图文件缺失时回退为主题背景色（不崩溃）
    void testMissingBackgroundImageFallsBackToPageColor();
    // 组件绘制在背景图之上
    void testComponentsDrawAboveBackgroundImage();
    // 卡片边框换图后，画布必须显示新图（图片缓存按路径失效）
    void testCardBorderImageSwapRefreshesCache();
};

namespace {

// 在临时项目目录里生成一张纯色 PNG 素材
bool writeAssetImage(const QString& strProjectDir, const QString& strRelativePath,
                     const QColor& rColor, const QSize& rSize)
{
    const QString strPath = QDir(strProjectDir).filePath(strRelativePath);
    if (!QDir().mkpath(QFileInfo(strPath).absolutePath())) {
        return false;
    }
    QImage image(rSize, QImage::Format_ARGB32);
    image.fill(rColor);
    return image.save(strPath, "PNG");
}

// 把整个场景渲染为图片并取指定像素颜色（0,0 为页面左上角）
QColor scenePixel(CanvasScene& rScene, const QPoint& rPosition)
{
    const QRectF sceneRect = rScene.sceneRect();
    QImage image(sceneRect.size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    rScene.render(&painter, QRectF(image.rect()), sceneRect);
    painter.end();
    return image.pixelColor(rPosition);
}

} // namespace

void TestCanvasScene::testBackgroundImageCoversPageColor()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QVERIFY(writeAssetImage(tempDir.path(), QStringLiteral("assets/bg.png"),
                            QColor(0, 0, 255), QSize(40, 20)));

    CanvasScene scene;
    scene.setProjectDirectory(tempDir.path());
    scene.setPageBackgroundColor(Qt::white);

    Page page;
    page.strName = QStringLiteral("图片页");
    page.size = QSize(40, 20);
    page.strBackgroundImage = QStringLiteral("assets/bg.png");
    scene.loadPage(page);

    // 整页都应是背景图的蓝色（页面尺寸 = 图片尺寸，正好铺满）
    QCOMPARE(scenePixel(scene, QPoint(5, 5)), QColor(0, 0, 255));
    QCOMPARE(scenePixel(scene, QPoint(35, 15)), QColor(0, 0, 255));
}

void TestCanvasScene::testPageColorWithoutBackgroundImage()
{
    CanvasScene scene;
    scene.setPageBackgroundColor(Qt::white);

    Page page;
    page.strName = QStringLiteral("空白页");
    page.size = QSize(40, 20);
    scene.loadPage(page);

    QCOMPARE(scenePixel(scene, QPoint(5, 5)), QColor(Qt::white));
}

void TestCanvasScene::testMissingBackgroundImageFallsBackToPageColor()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    CanvasScene scene;
    scene.setProjectDirectory(tempDir.path());
    scene.setPageBackgroundColor(Qt::white);

    Page page;
    page.strName = QStringLiteral("缺图页");
    page.size = QSize(40, 20);
    page.strBackgroundImage = QStringLiteral("assets/不存在.png");
    scene.loadPage(page);

    QCOMPARE(scenePixel(scene, QPoint(5, 5)), QColor(Qt::white));
}

void TestCanvasScene::testComponentsDrawAboveBackgroundImage()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QVERIFY(writeAssetImage(tempDir.path(), QStringLiteral("assets/bg.png"),
                            QColor(0, 0, 255), QSize(40, 20)));

    CanvasScene scene;
    scene.setProjectDirectory(tempDir.path());
    scene.setPageBackgroundColor(Qt::white);

    Page page;
    page.strName = QStringLiteral("图片页");
    page.size = QSize(40, 20);
    page.strBackgroundImage = QStringLiteral("assets/bg.png");

    Component shape;
    shape.strId = QStringLiteral("shape-1");
    shape.eType = E_COMPONENT_TYPE_SHAPE;
    shape.pos = QPointF(5, 5);
    shape.size = QSizeF(10, 10);
    shape.shapeData.fillColor = QColor(255, 0, 0);
    shape.shapeData.borderColor = QColor(255, 0, 0);
    shape.nZOrder = 1;
    page.vecComponents.append(shape);
    scene.loadPage(page);

    // 组件区域内为组件的红色，区域外仍为背景图的蓝色
    QCOMPARE(scenePixel(scene, QPoint(10, 10)), QColor(255, 0, 0));
    QCOMPARE(scenePixel(scene, QPoint(30, 15)), QColor(0, 0, 255));
}

void TestCanvasScene::testCardBorderImageSwapRefreshesCache()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QVERIFY(writeAssetImage(tempDir.path(), QStringLiteral("assets/red.png"),
                            QColor(255, 0, 0), QSize(40, 40)));
    QVERIFY(writeAssetImage(tempDir.path(), QStringLiteral("assets/green.png"),
                            QColor(0, 255, 0), QSize(40, 40)));

    CanvasScene scene;
    scene.setProjectDirectory(tempDir.path());

    Page page;
    page.size = QSize(40, 40);
    Component border;
    border.strId = QStringLiteral("border-1");
    border.eType = E_COMPONENT_TYPE_STICKER;
    border.stickerData.eStickerType = E_STICKER_TYPE_CARD_BORDER;
    border.stickerData.color = QColor(0, 0, 0);
    border.stickerData.strImagePath = QStringLiteral("assets/red.png");
    border.size = QSizeF(40, 40);
    border.nZOrder = 1;
    page.vecComponents.append(border);
    scene.loadPage(page);

    QCOMPARE(scenePixel(scene, QPoint(20, 20)), QColor(255, 0, 0));

    // 模拟「二次编辑 → 替换图片」：走组件数据变更入口后，缓存必须失效并显示新图
    const QVector<ComponentItem*> vecItems = scene.componentItems();
    QCOMPARE(vecItems.size(), 1);
    Component updated = vecItems.first()->component();
    updated.stickerData.strImagePath = QStringLiteral("assets/green.png");
    vecItems.first()->applyComponentData(updated);

    QCOMPARE(scenePixel(scene, QPoint(20, 20)), QColor(0, 255, 0));
}

int main(int argc, char* argv[])
{
    // 无显示环境（如 CI）下用 offscreen 平台插件，保证仍能创建 QApplication
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QStringLiteral("offscreen").toUtf8());
    }
    QApplication app(argc, argv);
    TestCanvasScene testCase;
    return QTest::qExec(&testCase, argc, argv);
}

#include "test_canvas_scene.moc"
