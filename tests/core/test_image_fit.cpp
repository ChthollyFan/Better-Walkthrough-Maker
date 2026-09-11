/**
 * @file test_image_fit.cpp
 * @author zhangweimu
 * @brief 图片适配（等比覆盖 + 取景偏移）的单元测试。
 *
 * 卡片边框内的图片靠这套算法实现「框内显示、框外隐藏」，
 * 页面背景图复用同一实现（居中取景），因此这里同时锁定两者的公共行为。
 */
#include <QtTest>

#include <QImage>
#include <QPainter>

#include "core/ImageFit.h"

using namespace bwm;

class TestImageFit : public QObject {
    Q_OBJECT

private slots:
    // 比例一致：源矩形为整张图
    void testExactFit();
    // 图片偏宽：默认居中裁剪，偏移可改取景位置
    void testHorizontalOffset();
    // 图片偏高：垂直偏移生效
    void testVerticalOffset();
    // 偏移越界时夹到合法区间
    void testOffsetClamped();
    // 非法参数返回空矩形
    void testInvalidArguments();
    // 实际绘制：不同取景位置取到图片的不同部分
    void testPaintCoverOffset();
};

void TestImageFit::testExactFit()
{
    const QRectF sourceRect = ImageFit::coverSourceRect(QSizeF(100, 200), QRectF(0, 0, 50, 100));
    QCOMPARE(sourceRect, QRectF(0, 0, 100, 200));
}

void TestImageFit::testHorizontalOffset()
{
    // 200x100 的图铺到 100x100：横向需裁掉一半，纵向不动
    const QSizeF imageSize(200, 100);
    const QRectF target(0, 0, 100, 100);

    QCOMPARE(ImageFit::coverSourceRect(imageSize, target, 0.5, 0.5), QRectF(50, 0, 100, 100));
    QCOMPARE(ImageFit::coverSourceRect(imageSize, target, 0.0, 0.5), QRectF(0, 0, 100, 100));
    QCOMPARE(ImageFit::coverSourceRect(imageSize, target, 1.0, 0.5), QRectF(100, 0, 100, 100));
    QCOMPARE(ImageFit::coverSourceRect(imageSize, target, 0.25, 0.5), QRectF(25, 0, 100, 100));
}

void TestImageFit::testVerticalOffset()
{
    // 100x200 的图铺到 100x100：纵向需裁掉一半
    const QSizeF imageSize(100, 200);
    const QRectF target(0, 0, 100, 100);

    QCOMPARE(ImageFit::coverSourceRect(imageSize, target, 0.5, 0.5), QRectF(0, 50, 100, 100));
    QCOMPARE(ImageFit::coverSourceRect(imageSize, target, 0.5, 1.0), QRectF(0, 100, 100, 100));
}

void TestImageFit::testOffsetClamped()
{
    const QSizeF imageSize(200, 100);
    const QRectF target(0, 0, 100, 100);
    // 负值与大于 1 的偏移夹到 0 / 1，避免取到图片之外的空白
    QCOMPARE(ImageFit::coverSourceRect(imageSize, target, -3.0, 0.5), QRectF(0, 0, 100, 100));
    QCOMPARE(ImageFit::coverSourceRect(imageSize, target, 9.0, 0.5), QRectF(100, 0, 100, 100));
}

void TestImageFit::testInvalidArguments()
{
    QVERIFY(ImageFit::coverSourceRect(QSizeF(0, 10), QRectF(0, 0, 10, 10)).isEmpty());
    QVERIFY(ImageFit::coverSourceRect(QSizeF(10, 10), QRectF(0, 0, 0, 10)).isEmpty());
}

void TestImageFit::testPaintCoverOffset()
{
    // 4x2 的图：左半红、右半绿
    QImage image(4, 2, QImage::Format_ARGB32);
    image.fill(QColor(255, 0, 0));
    for (int nX = 2; nX < 4; ++nX) {
        for (int nY = 0; nY < 2; ++nY) {
            image.setPixelColor(nX, nY, QColor(0, 255, 0));
        }
    }

    // 铺到 2x2：图片比目标宽一倍，只能显示其中一半
    QImage target(2, 2, QImage::Format_ARGB32);
    target.fill(Qt::white);
    QPainter painter(&target);
    ImageFit::paintCover(&painter, image, QRectF(0, 0, 2, 2), 0.0, 0.5);
    painter.end();
    QCOMPARE(target.pixelColor(0, 0), QColor(255, 0, 0));   // 贴左：显示红半

    QImage targetRight(2, 2, QImage::Format_ARGB32);
    targetRight.fill(Qt::white);
    QPainter painterRight(&targetRight);
    ImageFit::paintCover(&painterRight, image, QRectF(0, 0, 2, 2), 1.0, 0.5);
    painterRight.end();
    QCOMPARE(targetRight.pixelColor(0, 0), QColor(0, 255, 0));   // 贴右：显示绿半
}

QTEST_GUILESS_MAIN(TestImageFit)

#include "test_image_fit.moc"
