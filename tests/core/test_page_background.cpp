/**
 * @file test_page_background.cpp
 * @author zhangweimu
 * @brief 页面背景图「等比覆盖」绘制算法的单元测试（tests/ 镜像 src/core/）。
 */
#include <QtTest>

#include <QImage>
#include <QPainter>

#include "core/PageBackground.h"

using namespace bwm;

class TestPageBackground : public QObject {
    Q_OBJECT

private slots:
    // 图片与目标同比例：源矩形为整张图
    void testSameAspectRatio();
    // 图片比目标宽：左右居中裁剪
    void testCropWiderImage();
    // 图片比目标高：上下居中裁剪
    void testCropTallerImage();
    // 非法参数返回空矩形
    void testInvalidArguments();
    // 实际绘制：居中裁剪后目标区域被覆盖且取到正确像素
    void testPaintCover();
};

void TestPageBackground::testSameAspectRatio()
{
    const QRectF sourceRect = PageBackground::coverSourceRect(QSizeF(1000, 2000),
                                                              QRectF(0, 0, 100, 200));
    QCOMPARE(sourceRect, QRectF(0, 0, 1000, 2000));
}

void TestPageBackground::testCropWiderImage()
{
    // 1000x500 的图铺到 500x500：取满高，左右各裁 250
    const QRectF sourceRect = PageBackground::coverSourceRect(QSizeF(1000, 500),
                                                              QRectF(0, 0, 500, 500));
    QCOMPARE(sourceRect, QRectF(250, 0, 500, 500));
}

void TestPageBackground::testCropTallerImage()
{
    // 500x1000 的图铺到 500x500：取满宽，上下各裁 250
    const QRectF sourceRect = PageBackground::coverSourceRect(QSizeF(500, 1000),
                                                              QRectF(0, 0, 500, 500));
    QCOMPARE(sourceRect, QRectF(0, 250, 500, 500));
}

void TestPageBackground::testInvalidArguments()
{
    QVERIFY(PageBackground::coverSourceRect(QSizeF(0, 100), QRectF(0, 0, 100, 100)).isEmpty());
    QVERIFY(PageBackground::coverSourceRect(QSizeF(100, 100), QRectF(0, 0, 0, 100)).isEmpty());
}

void TestPageBackground::testPaintCover()
{
    // 4x2 的图：左半红、右半绿；铺到 2x2 目标后应取中间两列（x=1 红、x=2 绿）
    QImage image(4, 2, QImage::Format_ARGB32);
    image.fill(QColor(255, 0, 0));
    for (int nX = 2; nX < 4; ++nX) {
        for (int nY = 0; nY < 2; ++nY) {
            image.setPixelColor(nX, nY, QColor(0, 255, 0));
        }
    }

    QImage target(2, 2, QImage::Format_ARGB32);
    target.fill(Qt::white);
    QPainter painter(&target);
    PageBackground::paintCover(&painter, image, QRectF(0, 0, 2, 2));
    painter.end();

    QCOMPARE(target.pixelColor(0, 0), QColor(255, 0, 0));
    QCOMPARE(target.pixelColor(1, 0), QColor(0, 255, 0));

    // 空图片不绘制：目标保持原样
    QImage untouched(2, 2, QImage::Format_ARGB32);
    untouched.fill(Qt::white);
    QPainter emptyPainter(&untouched);
    PageBackground::paintCover(&emptyPainter, QImage(), QRectF(0, 0, 2, 2));
    emptyPainter.end();
    QCOMPARE(untouched.pixelColor(0, 0), QColor(Qt::white));
}

QTEST_GUILESS_MAIN(TestPageBackground)

#include "test_page_background.moc"
