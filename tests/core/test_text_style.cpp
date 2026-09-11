/**
 * @file test_text_style.cpp
 * @author zhangweimu
 * @brief 文本组件样式（对齐 / 加粗 / 颜色 / 不透明度）的单元测试。
 *
 * 覆盖：
 * - TextData 默认值与相等比较（不透明度参与比较）
 * - 序列化往返（不透明度写入 JSON；旧文件缺字段回退 100）
 * - 绘制：不透明度合成到颜色 alpha（50% 黑字在白底上不应是纯黑）
 */
#include <QtTest>

#include <QImage>
#include <QJsonObject>
#include <QPainter>

#include "core/Component.h"
#include "core/ComponentPainter.h"
#include "core/ComponentSerializer.h"

using namespace bwm;

class TestTextStyle : public QObject {
    Q_OBJECT

private slots:
    // 默认值：不透明度 100、左对齐、字号 24
    void testDefaults();
    // 相等比较包含不透明度（脏检测、撤销快照依赖它）
    void testEqualityIncludesOpacity();
    // 序列化往返：不透明度写入并读回
    void testSerializationRoundTrip();
    // 旧文件（无 opacity 字段）读回默认 100
    void testSerializationBackwardCompatible();
    // 绘制：50% 不透明度的黑字在白底上呈现灰色（不是纯黑）
    void testPaintAppliesOpacity();
    // 绘制：字体族/字号/加粗变化会影响墨迹范围
    void testPaintAppliesFontSize();
};

namespace {

// 把文本组件渲染到白底图片上（尺寸固定 300x60）
QImage renderText(const TextData& rText)
{
    Component component;
    component.eType = E_COMPONENT_TYPE_TEXT;
    component.textData = rText;
    const QRectF rect(0, 0, 300, 60);
    component.size = rect.size();

    QImage image(300, 60, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    ComponentPainter::paint(&painter, component, rect);
    painter.end();
    return image;
}

// 统计非白像素（文字的墨迹）数量
int countInk(const QImage& rImage)
{
    int nCount = 0;
    for (int nY = 0; nY < rImage.height(); ++nY) {
        for (int nX = 0; nX < rImage.width(); ++nX) {
            if (rImage.pixelColor(nX, nY) != QColor(Qt::white)) {
                ++nCount;
            }
        }
    }
    return nCount;
}

// 取图片中最深的像素（alpha 合成后文字核心处最暗）
QColor darkestPixel(const QImage& rImage)
{
    QColor darkest = QColor(Qt::white);
    for (int nY = 0; nY < rImage.height(); ++nY) {
        for (int nX = 0; nX < rImage.width(); ++nX) {
            const QColor pixel = rImage.pixelColor(nX, nY);
            if (pixel.red() + pixel.green() + pixel.blue()
                < darkest.red() + darkest.green() + darkest.blue()) {
                darkest = pixel;
            }
        }
    }
    return darkest;
}

} // namespace

void TestTextStyle::testDefaults()
{
    const TextData text;
    QCOMPARE(text.nFontSize, 24);
    QCOMPARE(text.nAlign, int(Qt::AlignLeft));
    QCOMPARE(text.nOpacityPercent, 100);
    QVERIFY(!text.bBold);
    QVERIFY(text.strFontFamily.isEmpty());   // 空 = 使用默认字体
    QCOMPARE(textDefaultFontFamily(), QStringLiteral("Microsoft YaHei"));
}

void TestTextStyle::testEqualityIncludesOpacity()
{
    TextData left;
    TextData right;
    QVERIFY(left == right);

    right.nOpacityPercent = 50;
    QVERIFY(left != right);

    right.nOpacityPercent = 100;
    right.strContent = QStringLiteral("内容");
    QVERIFY(left != right);
}

void TestTextStyle::testSerializationRoundTrip()
{
    Component component;
    component.eType = E_COMPONENT_TYPE_TEXT;
    component.textData.strContent = QStringLiteral("半透明文本");
    component.textData.nOpacityPercent = 35;

    const QJsonObject object = ComponentSerializer::toJson(component);
    const Component restored = ComponentSerializer::fromJson(object);
    QCOMPARE(restored.textData.strContent, QStringLiteral("半透明文本"));
    QCOMPARE(restored.textData.nOpacityPercent, 35);
    QVERIFY(restored.textData == component.textData);
}

void TestTextStyle::testSerializationBackwardCompatible()
{
    // 模拟旧版本文件：text 对象里没有 opacity 字段
    QJsonObject textObject;
    textObject.insert(QStringLiteral("content"), QStringLiteral("旧文本"));
    textObject.insert(QStringLiteral("fontSize"), 20);
    QJsonObject componentObject;
    componentObject.insert(QStringLiteral("type"), QStringLiteral("text"));
    componentObject.insert(QStringLiteral("text"), textObject);

    const Component restored = ComponentSerializer::fromJson(componentObject);
    QCOMPARE(restored.textData.strContent, QStringLiteral("旧文本"));
    QCOMPARE(restored.textData.nOpacityPercent, 100);   // 缺字段回退完全不透明
}

void TestTextStyle::testPaintAppliesOpacity()
{
    TextData opaque;
    opaque.strContent = QStringLiteral("不透明度测试");
    opaque.nFontSize = 32;
    opaque.nOpacityPercent = 100;

    TextData translucent = opaque;
    translucent.nOpacityPercent = 50;

    const QColor opaqueDarkest = darkestPixel(renderText(opaque));
    const QColor translucentDarkest = darkestPixel(renderText(translucent));

    // 完全不透明：文字核心接近纯黑
    QVERIFY2(opaqueDarkest.red() < 40,
             qPrintable(QStringLiteral("不透明文本应为近黑，实际 %1").arg(opaqueDarkest.name())));
    // 50% 不透明：黑字与白底合成后应明显变浅（约 128），但不是白色
    QVERIFY2(translucentDarkest.red() > 90 && translucentDarkest.red() < 210,
             qPrintable(QStringLiteral("50%% 不透明文本应呈灰色，实际 %1")
                            .arg(translucentDarkest.name())));
}

void TestTextStyle::testPaintAppliesFontSize()
{
    TextData small;
    small.strContent = QStringLiteral("字号测试");
    small.nFontSize = 12;

    TextData large = small;
    large.nFontSize = 40;

    const int nSmallInk = countInk(renderText(small));
    const int nLargeInk = countInk(renderText(large));
    QVERIFY2(nSmallInk > 0, "12px 文本未绘制");
    QVERIFY2(nLargeInk > nSmallInk,
             qPrintable(QStringLiteral("字号未生效：12px %1 像素，40px %2 像素")
                            .arg(nSmallInk).arg(nLargeInk)));
}

// 文本渲染需要 QGuiApplication（字体数据库），因此用 QTEST_MAIN
QTEST_MAIN(TestTextStyle)

#include "test_text_style.moc"
