/**
 * @file test_font_select_widget.cpp
 * @author zhangweimu
 * @brief 可复用字体组件 FontSelectWidget 的单元测试（设置-读取往返 / 透明度 / 信号 / 行显隐）。
 */
#include <QtTest>

#include <QApplication>
#include <QFontDatabase>
#include <QLabel>
#include <QSignalSpy>
#include <QToolButton>

#include "ui/FontSelectWidget.h"

using namespace bwm;

class TestFontSelectWidget : public QObject {
    Q_OBJECT

private slots:
    // 字体族 / 字号 / 加粗 设置后能原样读回
    void testFontRoundTrip();
    // 颜色（仅 RGB）与不透明度设置后能原样读回
    void testColorAndOpacityRoundTrip();
    // 传入带 alpha 的颜色时，透明度从 alpha 换算
    void testOpacityFromAlphaColor();
    // 字号越界时收敛到合法区间
    void testFontSizeClamp();
    // 属性变化发出 styleChanged 信号（外部刷新预览用）
    void testStyleChangedSignal();
    // 隐藏颜色/预览行后，控件仍可正常读写
    void testRowsVisibility();
};

void TestFontSelectWidget::testFontRoundTrip()
{
    FontSelectWidget widget;

    // 字号与加粗
    widget.setFontSize(30);
    widget.setBold(true);
    QCOMPARE(widget.fontSize(), 30);
    QVERIFY(widget.isBold());
    QCOMPARE(widget.font().pixelSize(), 30);
    QVERIFY(widget.font().bold());

    // 字体族：从系统字体库里挑一个与当前不同的，避免依赖特定字体是否安装
    const QString strCurrent = widget.fontFamily();
    QVERIFY(!strCurrent.isEmpty());
    const QStringList vecFamilies = QFontDatabase::families();
    QString strOther;
    for (const QString& strFamily : vecFamilies) {
        if (strFamily.compare(strCurrent, Qt::CaseInsensitive) != 0) {
            strOther = strFamily;
            break;
        }
    }
    if (!strOther.isEmpty()) {
        widget.setFontFamily(strOther);
        QCOMPARE(widget.fontFamily(), strOther);
    }
    // 空字体族应被忽略（由调用方决定回退值）
    widget.setFontFamily(QString());
    QCOMPARE(widget.fontFamily(), strOther.isEmpty() ? strCurrent : strOther);

    // setFont 一次性设置三项
    QFont font(QStringLiteral("SimSun"));
    font.setPixelSize(22);
    font.setBold(false);
    widget.setFont(font);
    if (QFontDatabase::families().contains(QStringLiteral("SimSun"))) {
        QCOMPARE(widget.fontFamily(), QStringLiteral("SimSun"));
    }
    QCOMPARE(widget.fontSize(), 22);
    QVERIFY(!widget.isBold());
}

void TestFontSelectWidget::testColorAndOpacityRoundTrip()
{
    FontSelectWidget widget;
    widget.setColor(QColor(12, 34, 56));
    widget.setOpacityPercent(40);

    QCOMPARE(widget.color(), QColor(12, 34, 56));
    QCOMPARE(widget.opacityPercent(), 40);
    // 颜色只保留 RGB（透明度由独立控件负责）
    QCOMPARE(widget.color().alpha(), 255);
}

void TestFontSelectWidget::testOpacityFromAlphaColor()
{
    FontSelectWidget widget;
    // alpha 128 ≈ 50%
    widget.setColor(QColor(255, 0, 0, 128));
    QCOMPARE(widget.opacityPercent(), 50);
    QCOMPARE(widget.color(), QColor(255, 0, 0));

    // 不带 alpha 的颜色不应改动已有透明度
    widget.setOpacityPercent(80);
    widget.setColor(QColor(0, 255, 0));
    QCOMPARE(widget.opacityPercent(), 80);
}

void TestFontSelectWidget::testFontSizeClamp()
{
    FontSelectWidget widget;
    widget.setFontSize(1);
    QCOMPARE(widget.fontSize(), 6);
    widget.setFontSize(9999);
    QCOMPARE(widget.fontSize(), 400);
}

void TestFontSelectWidget::testStyleChangedSignal()
{
    FontSelectWidget widget;
    QSignalSpy styleSpy(&widget, &FontSelectWidget::styleChanged);
    QSignalSpy fontSpy(&widget, &FontSelectWidget::fontChanged);
    QSignalSpy colorSpy(&widget, &FontSelectWidget::colorChanged);

    widget.setFontSize(32);              // setter 不触发信号（仅界面操作触发）
    QCOMPARE(styleSpy.count(), 0);

    widget.setColor(QColor(10, 20, 30)); // setColor 主动发信号
    QCOMPARE(colorSpy.count(), 1);
    QCOMPARE(styleSpy.count(), 1);

    widget.setOpacityPercent(50);
    QCOMPARE(colorSpy.count(), 2);
    QCOMPARE(styleSpy.count(), 2);
    Q_UNUSED(fontSpy);
}

void TestFontSelectWidget::testRowsVisibility()
{
    FontSelectWidget widget;

    // 颜色行按钮：唯一一个 QToolButton
    QToolButton* pColorButton = widget.findChild<QToolButton*>();
    QVERIFY(pColorButton != nullptr);
    QVERIFY(pColorButton->isVisibleTo(&widget));

    // 预览行：带边框的 QLabel
    QLabel* pPreviewLabel = nullptr;
    for (QLabel* pLabel : widget.findChildren<QLabel*>()) {
        if (pLabel->frameShape() != QFrame::NoFrame) {
            pPreviewLabel = pLabel;
            break;
        }
    }
    QVERIFY(pPreviewLabel != nullptr);

    widget.setColorRowsVisible(false);
    QVERIFY(!pColorButton->isVisibleTo(&widget));

    widget.setPreviewVisible(false);
    QVERIFY(!pPreviewLabel->isVisibleTo(&widget));

    // 隐藏后读写仍正常
    widget.setColor(QColor(1, 2, 3));
    QCOMPARE(widget.color(), QColor(1, 2, 3));

    widget.setColorRowsVisible(true);
    widget.setPreviewVisible(true);
    QVERIFY(pColorButton->isVisibleTo(&widget));
    QVERIFY(pPreviewLabel->isVisibleTo(&widget));
}

int main(int argc, char* argv[])
{
    // 无显示环境（如 CI）下用 offscreen 平台插件，保证仍能创建 QApplication
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QStringLiteral("offscreen").toUtf8());
    }
    QApplication app(argc, argv);
    TestFontSelectWidget testCase;
    return QTest::qExec(&testCase, argc, argv);
}

#include "test_font_select_widget.moc"
