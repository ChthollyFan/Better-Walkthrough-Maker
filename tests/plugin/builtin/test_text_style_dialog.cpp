/**
 * @file test_text_style_dialog.cpp
 * @author zhangweimu
 * @brief 文本样式对话框的单元测试（构造、默认值、数据往返、越界收敛）。
 *
 * 对话框同时被「插入 → 文本」与「双击编辑文本」复用，
 * 因此这里直接构造对话框验证「设置 → 读回」的往返正确性。
 */
#include <QtTest>

#include <QApplication>
#include <QFontDatabase>

#include "core/Component.h"
#include "plugin/builtin/TextStyleDialog.h"

using namespace bwm;

class TestTextStyleDialog : public QObject {
    Q_OBJECT

private slots:
    // 默认构造（空 TextData）：内容为空、左对齐、不透明度 100
    void testConstructWithDefaults();
    // 自定义数据（内容/对齐/字体/字号/加粗/颜色/不透明度）原样读回
    void testDataRoundTrip();
    // 不透明度越界时收敛到 0~100
    void testOpacityClamp();
    // 字号越界时收敛
    void testFontSizeClamp();
    // 未改动水平对齐时保留原值（历史 1 / 129 两种左对齐都不跳变）
    void testAlignValuePreservedWhenUnchanged();
};

void TestTextStyleDialog::testConstructWithDefaults()
{
    TextStyleDialog dialog(nullptr, TextData());
    const TextData result = dialog.textData();
    QVERIFY(result.strContent.isEmpty());
    // 左对齐沿用历史默认值（仅水平左对齐），保证插入新文本的排版与旧版本一致
    QCOMPARE(result.nAlign, int(Qt::AlignLeft));
    QCOMPARE(result.nFontSize, 24);
    QVERIFY(!result.bBold);
    QCOMPARE(result.nOpacityPercent, 100);
    // 空字体族由对话框解析为默认字体（绘制时同一来源）
    QVERIFY(!result.strFontFamily.isEmpty());
}

void TestTextStyleDialog::testDataRoundTrip()
{
    TextData source;
    source.strContent = QStringLiteral("出血流装备推荐");
    source.nAlign = int(Qt::AlignHCenter | Qt::AlignVCenter);
    source.nFontSize = 36;
    source.bBold = true;
    source.color = QColor(200, 30, 30);
    source.nOpacityPercent = 45;
    source.strFontFamily = textDefaultFontFamily();

    TextStyleDialog dialog(nullptr, source);
    const TextData result = dialog.textData();

    QCOMPARE(result.strContent, source.strContent);
    QCOMPARE(result.nAlign, source.nAlign);
    QCOMPARE(result.nFontSize, 36);
    QVERIFY(result.bBold);
    QCOMPARE(result.color.rgb(), QColor(200, 30, 30).rgb());
    QCOMPARE(result.nOpacityPercent, 45);
    // 字体族：部分环境（如 offscreen 无字体库）Qt 会自行回退，系统有该字体时才严格比对
    QVERIFY(!result.strFontFamily.isEmpty());
    if (QFontDatabase::families().contains(source.strFontFamily)) {
        QCOMPARE(result.strFontFamily, source.strFontFamily);
    }
}

void TestTextStyleDialog::testOpacityClamp()
{
    TextData source;
    source.nOpacityPercent = 300;
    TextStyleDialog over(nullptr, source);
    QCOMPARE(over.textData().nOpacityPercent, 100);

    source.nOpacityPercent = -10;
    TextStyleDialog under(nullptr, source);
    QVERIFY(under.textData().nOpacityPercent >= 0);
}

void TestTextStyleDialog::testFontSizeClamp()
{
    TextData source;
    source.nFontSize = 9999;
    TextStyleDialog dialog(nullptr, source);
    const int nSize = dialog.textData().nFontSize;
    QVERIFY(nSize >= 6);
    QVERIFY(nSize <= 400);
}

void TestTextStyleDialog::testAlignValuePreservedWhenUnchanged()
{
    // 仅左对齐（历史默认）
    TextData legacy;
    legacy.nAlign = int(Qt::AlignLeft);
    TextStyleDialog legacyDialog(nullptr, legacy);
    QCOMPARE(legacyDialog.textData().nAlign, int(Qt::AlignLeft));

    // 左 + 垂直居中（旧「编辑文本」对话框写入过的值）：未改对齐应原样保留
    TextData leftVCenter;
    leftVCenter.nAlign = int(Qt::AlignLeft | Qt::AlignVCenter);
    TextStyleDialog leftDialog(nullptr, leftVCenter);
    QCOMPARE(leftDialog.textData().nAlign, int(Qt::AlignLeft | Qt::AlignVCenter));
}

int main(int argc, char* argv[])
{
    // 无显示环境（如 CI）下用 offscreen 平台插件，保证仍能创建 QApplication
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QStringLiteral("offscreen").toUtf8());
    }
    QApplication app(argc, argv);
    TestTextStyleDialog testCase;
    return QTest::qExec(&testCase, argc, argv);
}

#include "test_text_style_dialog.moc"
