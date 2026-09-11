/**
 * @file test_card_border_dialog.cpp
 * @author zhangweimu
 * @brief 卡片边框设置对话框的单元测试（构造与数据往返）。
 *
 * 回归背景：对话框构造时曾先给取景滑块赋初值、再创建预览控件，
 * setValue 触发 valueChanged → 刷新预览 → 访问尚未创建的预览控件，
 * 表现为「点击插入/卡片边框立即闪退」。本测试直接构造对话框（含非默认取景位置），
 * 任何构造期的空指针访问都会让用例崩溃或失败。
 */
#include <QtTest>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>

#include "core/Component.h"
#include "plugin/builtin/CardBorderDialog.h"

using namespace bwm;

class TestCardBorderDialog : public QObject {
    Q_OBJECT

private slots:
    // 默认构造：矩形、无图片、居中取景
    void testConstructDefault();
    // 圆形 + 已设图片与非居中取景：构造期不崩溃，数据正确回读
    void testConstructWithImageAndOffset();
    // 四种形状都能构造，且只有正方形/圆形需要把组件尺寸归一为 1:1
    void testShapesAndSquareSizeFlag();
};

namespace {

// 生成一张测试图片
bool writeTestImage(const QString& strProjectDir)
{
    const QString strPath = QDir(strProjectDir).filePath(QStringLiteral("assets/pic.png"));
    if (!QDir().mkpath(QFileInfo(strPath).absolutePath())) {
        return false;
    }
    QImage image(120, 60, QImage::Format_ARGB32);
    image.fill(QColor(30, 120, 200));
    return image.save(strPath, "PNG");
}

} // namespace

void TestCardBorderDialog::testConstructDefault()
{
    StickerData sticker;
    sticker.eStickerType = E_STICKER_TYPE_CARD_BORDER;

    CardBorderDialog dialog(nullptr, sticker, QString());
    const StickerData result = dialog.stickerData();
    QCOMPARE(result.eStickerType, E_STICKER_TYPE_CARD_BORDER);
    QCOMPARE(result.eBorderShape, E_CARD_BORDER_SHAPE_RECTANGLE);
    QVERIFY(result.strImagePath.isEmpty());
    QVERIFY(!dialog.needsSquareSize());
}

void TestCardBorderDialog::testConstructWithImageAndOffset()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QVERIFY(writeTestImage(tempDir.path()));

    StickerData sticker;
    sticker.eStickerType = E_STICKER_TYPE_CARD_BORDER;
    sticker.eBorderShape = E_CARD_BORDER_SHAPE_CIRCLE;
    sticker.strImagePath = QStringLiteral("assets/pic.png");
    // 非默认取景位置：构造时会给滑块赋初值，正是曾经崩溃的路径
    sticker.dImageOffsetX = 0.25;
    sticker.dImageOffsetY = 0.75;

    CardBorderDialog dialog(nullptr, sticker, tempDir.path());
    const StickerData result = dialog.stickerData();
    QCOMPARE(result.eBorderShape, E_CARD_BORDER_SHAPE_CIRCLE);
    QCOMPARE(result.strImagePath, QStringLiteral("assets/pic.png"));
    QCOMPARE(result.dImageOffsetX, 0.25);
    QCOMPARE(result.dImageOffsetY, 0.75);
    QVERIFY(dialog.needsSquareSize());
}

void TestCardBorderDialog::testShapesAndSquareSizeFlag()
{
    struct Case {
        E_CARD_BORDER_SHAPE eShape;
        bool bNeedsSquareSize;
    };
    const QVector<Case> vecCases = {
        {E_CARD_BORDER_SHAPE_RECTANGLE, false},
        {E_CARD_BORDER_SHAPE_SQUARE, true},
        {E_CARD_BORDER_SHAPE_CIRCLE, true},
        {E_CARD_BORDER_SHAPE_ELLIPSE, false},
    };
    for (const Case& rCase : vecCases) {
        StickerData sticker;
        sticker.eStickerType = E_STICKER_TYPE_CARD_BORDER;
        sticker.eBorderShape = rCase.eShape;
        CardBorderDialog dialog(nullptr, sticker, QString());
        QCOMPARE(dialog.stickerData().eBorderShape, rCase.eShape);
        QCOMPARE(dialog.needsSquareSize(), rCase.bNeedsSquareSize);
    }
}

int main(int argc, char* argv[])
{
    // 无显示环境（如 CI）下用 offscreen 平台插件，保证仍能创建 QApplication
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QStringLiteral("offscreen").toUtf8());
    }
    QApplication app(argc, argv);
    TestCardBorderDialog testCase;
    return QTest::qExec(&testCase, argc, argv);
}

#include "test_card_border_dialog.moc"
