/**
 * @file test_author_mark_dialog.cpp
 * @author zhangweimu
 * @brief 署名设置对话框的单元测试（构造、默认值、样式往返）。
 *
 * 对话框代码属于主程序目标（src/app/），本测试按 test_canvas_scene 的做法
 * 单独编译该源文件后测试，避免为测试把对话框下沉到 core。
 */
#include <QtTest>

#include <QApplication>
#include <QFontDatabase>

#include "app/dialogs/AuthorMarkDialog.h"
#include "core/AuthorMarkStyle.h"

using namespace bwm;

class TestAuthorMarkDialog : public QObject {
    Q_OBJECT

private slots:
    // 默认样式构造后原样读回（含右下角默认位置）
    void testDefaultStyleRoundTrip();
    // 自定义样式（左上角 + 字号 + 加粗 + 颜色 + 不透明度）原样读回
    void testCustomStyleRoundTrip();
    // 字号越界时收敛
    void testFontSizeClamp();
};

// 取一个系统里确定存在的字体族，避免测试依赖特定字体是否安装
QString availableFontFamily()
{
    const QStringList vecFamilies = QFontDatabase::families();
    return vecFamilies.isEmpty() ? QString() : vecFamilies.first();
}

void TestAuthorMarkDialog::testDefaultStyleRoundTrip()
{
    AuthorMarkDialog dialog(nullptr, AuthorMarkStyle(), QStringLiteral("测试作者"));
    const AuthorMarkStyle style = dialog.style();
    QCOMPARE(style.ePosition, E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT);
    QCOMPARE(style.nFontSize, 18);
    QVERIFY(!style.bBold);
    QCOMPARE(style.color.rgb(), QColor(0, 0, 0).rgb());
    QCOMPARE(style.nOpacityPercent, 63);
    // 默认字体族：样式里为空 → 对话框填入默认字体族。
    // 部分环境（如 offscreen 无字体库）Qt 会自行回退，因此只要求结果非空，
    // 系统确实装有默认字体时再严格比对。
    const QString strResolved = style.resolvedFontFamily();
    QVERIFY(!strResolved.isEmpty());
    if (QFontDatabase::families().contains(AuthorMarkStyle::defaultFontFamily())) {
        QCOMPARE(strResolved, AuthorMarkStyle::defaultFontFamily());
    }
}

void TestAuthorMarkDialog::testCustomStyleRoundTrip()
{
    AuthorMarkStyle source;
    source.ePosition = E_AUTHOR_MARK_POSITION_TOP_LEFT;
    source.strFontFamily = availableFontFamily();
    source.nFontSize = 32;
    source.bBold = true;
    source.color = QColor(200, 30, 40);
    source.nOpacityPercent = 25;

    AuthorMarkDialog dialog(nullptr, source, QStringLiteral("测试作者"));
    const AuthorMarkStyle result = dialog.style();

    QCOMPARE(result.ePosition, E_AUTHOR_MARK_POSITION_TOP_LEFT);
    QCOMPARE(result.nFontSize, 32);
    QVERIFY(result.bBold);
    QCOMPARE(result.color.rgb(), QColor(200, 30, 40).rgb());
    QCOMPARE(result.nOpacityPercent, 25);
    if (!source.strFontFamily.isEmpty()) {
        QCOMPARE(result.strFontFamily, source.strFontFamily);
    }
}

void TestAuthorMarkDialog::testFontSizeClamp()
{
    AuthorMarkStyle source;
    source.nFontSize = 9999;   // 超出组件范围
    AuthorMarkDialog dialog(nullptr, source, QString());
    // 构造时对话框把字号塞进输入框（收敛到上限），读回不应越界
    QVERIFY(dialog.style().nFontSize <= 400);
    QVERIFY(dialog.style().nFontSize >= 6);
}

int main(int argc, char* argv[])
{
    // 无显示环境（如 CI）下用 offscreen 平台插件，保证仍能创建 QApplication
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QStringLiteral("offscreen").toUtf8());
    }
    QApplication app(argc, argv);
    TestAuthorMarkDialog testCase;
    return QTest::qExec(&testCase, argc, argv);
}

#include "test_author_mark_dialog.moc"
