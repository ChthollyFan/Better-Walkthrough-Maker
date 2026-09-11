/**
 * @file test_author_mark_style.cpp
 * @author zhangweimu
 * @brief 署名水印样式模型的单元测试（默认值 / 区间收敛 / 位置字符串互转）。
 */
#include <QtTest>

#include "core/AuthorMarkStyle.h"

using namespace bwm;

class TestAuthorMarkStyle : public QObject {
    Q_OBJECT

private slots:
    // 默认样式必须等价旧版硬编码水印，保证未设置过的用户导出结果不变
    void testDefaultsMatchLegacyWatermark();
    // 四种位置的字符串互转可往返
    void testPositionStringRoundTrip();
    // 未知/损坏的位置字符串回退右下角
    void testUnknownPositionFallsBackToBottomRight();
    // 字号与不透明度越界时收敛
    void testClampRange();
    // 空字体族回退默认字体
    void testResolvedFontFamily();
    // 相等比较忽略颜色 alpha（颜色只由 RGB + 不透明度决定）
    void testEqualityIgnoresColorAlpha();
    // 位置中文名齐全（下拉框显示用）
    void testPositionDisplayNames();
};

void TestAuthorMarkStyle::testDefaultsMatchLegacyWatermark()
{
    const AuthorMarkStyle style;
    QCOMPARE(style.ePosition, E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT);
    QVERIFY(style.strFontFamily.isEmpty());   // 空 = 使用默认字体
    QCOMPARE(style.resolvedFontFamily(), QStringLiteral("Microsoft YaHei"));
    QCOMPARE(style.nFontSize, 18);
    QVERIFY(!style.bBold);
    QCOMPARE(style.color.rgb(), QColor(0, 0, 0).rgb());
    QCOMPARE(style.nOpacityPercent, 63);
}

void TestAuthorMarkStyle::testPositionStringRoundTrip()
{
    const QVector<E_AUTHOR_MARK_POSITION> vecPositions = {
        E_AUTHOR_MARK_POSITION_TOP_LEFT,
        E_AUTHOR_MARK_POSITION_TOP_RIGHT,
        E_AUTHOR_MARK_POSITION_BOTTOM_LEFT,
        E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT,
    };
    for (const E_AUTHOR_MARK_POSITION ePosition : vecPositions) {
        const QString strValue = authorMarkPositionToString(ePosition);
        QVERIFY(!strValue.isEmpty());
        QCOMPARE(authorMarkPositionFromString(strValue), ePosition);
    }
    // 字符串值本身保持英文键名，便于手改配置文件
    QCOMPARE(authorMarkPositionToString(E_AUTHOR_MARK_POSITION_TOP_LEFT),
             QStringLiteral("top-left"));
}

void TestAuthorMarkStyle::testUnknownPositionFallsBackToBottomRight()
{
    QCOMPARE(authorMarkPositionFromString(QString()),
             E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT);
    QCOMPARE(authorMarkPositionFromString(QStringLiteral("middle")),
             E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT);
}

void TestAuthorMarkStyle::testClampRange()
{
    AuthorMarkStyle style;
    style.nFontSize = 1;
    style.nOpacityPercent = -20;
    style.clamp();
    QCOMPARE(style.nFontSize, 6);
    QCOMPARE(style.nOpacityPercent, 0);

    style.nFontSize = 9999;
    style.nOpacityPercent = 300;
    style.clamp();
    QCOMPARE(style.nFontSize, 400);
    QCOMPARE(style.nOpacityPercent, 100);
}

void TestAuthorMarkStyle::testResolvedFontFamily()
{
    AuthorMarkStyle style;
    style.strFontFamily = QStringLiteral("  ");
    QCOMPARE(style.resolvedFontFamily(), AuthorMarkStyle::defaultFontFamily());

    style.strFontFamily = QStringLiteral("SimSun");
    QCOMPARE(style.resolvedFontFamily(), QStringLiteral("SimSun"));
}

void TestAuthorMarkStyle::testEqualityIgnoresColorAlpha()
{
    AuthorMarkStyle left;
    AuthorMarkStyle right;
    left.color = QColor(255, 0, 0, 255);
    right.color = QColor(255, 0, 0, 40);
    QVERIFY(left == right);   // alpha 由 nOpacityPercent 负责，颜色只比 RGB

    right.nOpacityPercent = 30;
    QVERIFY(left != right);
}

void TestAuthorMarkStyle::testPositionDisplayNames()
{
    QCOMPARE(authorMarkPositionDisplayName(E_AUTHOR_MARK_POSITION_TOP_LEFT),
             QStringLiteral("左上角"));
    QCOMPARE(authorMarkPositionDisplayName(E_AUTHOR_MARK_POSITION_TOP_RIGHT),
             QStringLiteral("右上角"));
    QCOMPARE(authorMarkPositionDisplayName(E_AUTHOR_MARK_POSITION_BOTTOM_LEFT),
             QStringLiteral("左下角"));
    QCOMPARE(authorMarkPositionDisplayName(E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT),
             QStringLiteral("右下角"));
}

QTEST_MAIN(TestAuthorMarkStyle)

#include "test_author_mark_style.moc"
