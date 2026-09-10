/**
 * @file test_card_border.cpp
 * @author zhangweimu
 * @brief 卡片边框（矩形/正方形/圆形/椭圆）的单元测试：序列化与绘制几何。
 *
 * 绘制断言通过把组件渲染到 QImage 后取像素完成：
 * 正方形/圆形取组件内接正方形（居中），因此组件较宽时左右两侧不应有边框像素；
 * 圆形/椭圆的四角位于图形之外，同样不应有边框像素。
 */
#include <QtTest>

#include <QColor>
#include <QImage>
#include <QPainter>

#include "core/Component.h"
#include "core/ComponentPainter.h"
#include "core/ComponentSerializer.h"

using namespace bwm;

class TestCardBorder : public QObject {
    Q_OBJECT

private slots:
    // 四种形状与颜色序列化往返一致；缺字段（旧文件）按矩形处理
    void testSerializationRoundtrip();
    // 非卡片边框贴纸不写形状字段
    void testShapeFieldWrittenOnlyForCardBorder();
    // 矩形：填满组件矩形，边上有线、中心为空
    void testRectangleBorder();
    // 正方形：取内接正方形，组件较宽时左右两侧无线条
    void testSquareBorder();
    // 圆形：取内接圆，四角无线条
    void testCircleBorder();
    // 椭圆：填满组件矩形，四角无线条
    void testEllipseBorder();
};

namespace {

const QColor kBorderColor(255, 0, 0);   // 纯红，便于与白底区分

// 渲染一个指定形状的卡片边框到白底图片
QImage renderBorder(const QSize& rSize, E_CARD_BORDER_SHAPE eShape)
{
    Component component;
    component.eType = E_COMPONENT_TYPE_STICKER;
    component.stickerData.eStickerType = E_STICKER_TYPE_CARD_BORDER;
    component.stickerData.eBorderShape = eShape;
    component.stickerData.color = kBorderColor;
    component.size = QSizeF(rSize);

    QImage image(rSize, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    ComponentPainter::paint(&painter, component, QRectF(QPointF(0, 0), QSizeF(rSize)));
    painter.end();
    return image;
}

// 区域内是否存在边框色像素（抗锯齿会产生偏粉的像素，故放宽绿色/蓝色阈值）
bool hasBorderPixel(const QImage& rImage, const QRect& rArea)
{
    for (int nY = rArea.top(); nY <= rArea.bottom(); ++nY) {
        for (int nX = rArea.left(); nX <= rArea.right(); ++nX) {
            if (!rImage.rect().contains(nX, nY)) {
                continue;
            }
            const QColor color = rImage.pixelColor(nX, nY);
            if (color.red() > 150 && color.green() < 150 && color.blue() < 150) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

void TestCardBorder::testSerializationRoundtrip()
{
    const QVector<E_CARD_BORDER_SHAPE> vecShapes = {
        E_CARD_BORDER_SHAPE_RECTANGLE, E_CARD_BORDER_SHAPE_SQUARE,
        E_CARD_BORDER_SHAPE_CIRCLE, E_CARD_BORDER_SHAPE_ELLIPSE,
    };
    for (const E_CARD_BORDER_SHAPE eShape : vecShapes) {
        Component component;
        component.strId = QStringLiteral("sticker-1");
        component.eType = E_COMPONENT_TYPE_STICKER;
        component.stickerData.eStickerType = E_STICKER_TYPE_CARD_BORDER;
        component.stickerData.eBorderShape = eShape;
        component.stickerData.color = QColor(10, 20, 30);

        const Component parsed = ComponentSerializer::fromJson(ComponentSerializer::toJson(component));
        QCOMPARE(parsed.eType, E_COMPONENT_TYPE_STICKER);
        QCOMPARE(parsed.stickerData.eStickerType, E_STICKER_TYPE_CARD_BORDER);
        QCOMPARE(parsed.stickerData.eBorderShape, eShape);
        QCOMPARE(parsed.stickerData.color, QColor(10, 20, 30));
    }

    // 旧文件缺 borderShape 字段：按矩形处理（与旧版本视觉一致），不得报错
    const QString strLegacy = QStringLiteral(R"({
        "id": "old-1", "type": "sticker",
        "sticker": { "stickerType": "card_border", "color": "#123456" }
    })");
    const Component legacy = ComponentSerializer::fromJson(
        QJsonDocument::fromJson(strLegacy.toUtf8()).object());
    QCOMPARE(legacy.stickerData.eStickerType, E_STICKER_TYPE_CARD_BORDER);
    QCOMPARE(legacy.stickerData.eBorderShape, E_CARD_BORDER_SHAPE_RECTANGLE);
}

void TestCardBorder::testShapeFieldWrittenOnlyForCardBorder()
{
    Component divider;
    divider.eType = E_COMPONENT_TYPE_STICKER;
    divider.stickerData.eStickerType = E_STICKER_TYPE_DIVIDER;
    const QJsonObject dividerObject = ComponentSerializer::toJson(divider);
    QVERIFY(!dividerObject.value(QStringLiteral("sticker")).toObject()
                 .contains(QStringLiteral("borderShape")));

    Component cardBorder;
    cardBorder.eType = E_COMPONENT_TYPE_STICKER;
    cardBorder.stickerData.eStickerType = E_STICKER_TYPE_CARD_BORDER;
    cardBorder.stickerData.eBorderShape = E_CARD_BORDER_SHAPE_CIRCLE;
    const QJsonObject borderObject = ComponentSerializer::toJson(cardBorder);
    QCOMPARE(borderObject.value(QStringLiteral("sticker")).toObject()
                 .value(QStringLiteral("borderShape")).toString(),
             QStringLiteral("circle"));
}

void TestCardBorder::testRectangleBorder()
{
    const QImage image = renderBorder(QSize(200, 100), E_CARD_BORDER_SHAPE_RECTANGLE);
    // 四条边的中部有线条
    QVERIFY(hasBorderPixel(image, QRect(90, 0, 20, 4)));
    QVERIFY(hasBorderPixel(image, QRect(90, 96, 20, 4)));
    QVERIFY(hasBorderPixel(image, QRect(0, 40, 4, 20)));
    QVERIFY(hasBorderPixel(image, QRect(196, 40, 4, 20)));
    // 中心为空（只有边框线，不填充）
    QVERIFY(!hasBorderPixel(image, QRect(80, 40, 40, 20)));
}

void TestCardBorder::testSquareBorder()
{
    // 组件 200x100 → 内接正方形 100x100 居中（x 从 50 到 150）
    const QImage image = renderBorder(QSize(200, 100), E_CARD_BORDER_SHAPE_SQUARE);
    QVERIFY(hasBorderPixel(image, QRect(95, 0, 10, 4)));     // 正方形顶边
    QVERIFY(hasBorderPixel(image, QRect(50, 40, 4, 20)));    // 正方形左边
    // 正方形之外（左右两侧）不应有线条
    QVERIFY(!hasBorderPixel(image, QRect(0, 0, 45, 100)));
    QVERIFY(!hasBorderPixel(image, QRect(155, 0, 45, 100)));
}

void TestCardBorder::testCircleBorder()
{
    const QImage image = renderBorder(QSize(100, 100), E_CARD_BORDER_SHAPE_CIRCLE);
    QVERIFY(hasBorderPixel(image, QRect(45, 0, 10, 4)));    // 圆顶部
    QVERIFY(hasBorderPixel(image, QRect(0, 45, 4, 10)));    // 圆左侧
    // 四角在圆外，不应有线条（取 10x10 角块：再大就会贴到圆弧上）
    QVERIFY(!hasBorderPixel(image, QRect(0, 0, 10, 10)));
    QVERIFY(!hasBorderPixel(image, QRect(90, 0, 10, 10)));
    QVERIFY(!hasBorderPixel(image, QRect(0, 90, 10, 10)));
    QVERIFY(!hasBorderPixel(image, QRect(90, 90, 10, 10)));
}

void TestCardBorder::testEllipseBorder()
{
    const QImage image = renderBorder(QSize(200, 100), E_CARD_BORDER_SHAPE_ELLIPSE);
    QVERIFY(hasBorderPixel(image, QRect(95, 0, 10, 4)));     // 椭圆顶部
    QVERIFY(hasBorderPixel(image, QRect(0, 45, 4, 10)));     // 椭圆最左
    // 四角在椭圆外，不应有线条。注意椭圆是扁的：x=20 处弧线已逼近 y=20，
    // 故角块取 10x10，避免断言区域压到弧线上。
    QVERIFY(!hasBorderPixel(image, QRect(0, 0, 10, 10)));
    QVERIFY(!hasBorderPixel(image, QRect(190, 0, 10, 10)));
    QVERIFY(!hasBorderPixel(image, QRect(0, 90, 10, 10)));
    QVERIFY(!hasBorderPixel(image, QRect(190, 90, 10, 10)));
}

QTEST_GUILESS_MAIN(TestCardBorder)

#include "test_card_border.moc"
