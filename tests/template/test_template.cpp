/**
 * @file test_template.cpp
 * @author zhangweimu
 * @brief 模板序列化与内置模板的单元测试。
 */
#include <QtTest>

#include <QTemporaryDir>

#include "core/Template.h"
#include "template/BuiltinTemplates.h"

using namespace bwm;

class TestTemplate : public QObject {
    Q_OBJECT

private slots:
    // 模板序列化往返后字段一致
    void testRoundtrip();
    // 文件写入后读回一致
    void testFileRoundtrip();
    // 内置模板：六类齐全且每类至少一页
    void testBuiltinTemplates();
    // 模板解析失败返回错误
    void testInvalidJson();
};

void TestTemplate::testRoundtrip()
{
    Template t;
    t.strName = QStringLiteral("装备推荐");
    t.eType = E_WALKTHROUGH_TYPE_EQUIPMENT;
    t.strDescription = QStringLiteral("测试模板");

    Component text;
    text.strId = QStringLiteral("text-1");
    text.eType = E_COMPONENT_TYPE_TEXT;
    text.pos = QPointF(50, 60);
    text.textData.strContent = QStringLiteral("装备名");
    text.textData.nFontSize = 30;

    Component sticker;
    sticker.strId = QStringLiteral("sticker-1");
    sticker.eType = E_COMPONENT_TYPE_STICKER;
    sticker.stickerData.eStickerType = E_STICKER_TYPE_STAR_RATING;
    sticker.stickerData.color = QColor(200, 50, 50);

    Page page;
    page.strName = QStringLiteral("页面 1");
    page.size = QSize(1080, 1440);
    page.vecComponents = {text, sticker};
    t.vecPages.append(page);

    const QString strJson = TemplateSerializer::toJson(t);
    Template parsed;
    QString strErrorMessage;
    QVERIFY2(TemplateSerializer::fromJson(strJson, &parsed, &strErrorMessage), qPrintable(strErrorMessage));

    QCOMPARE(parsed.strName, QStringLiteral("装备推荐"));
    QCOMPARE(parsed.eType, E_WALKTHROUGH_TYPE_EQUIPMENT);
    QCOMPARE(parsed.vecPages.size(), 1);
    QCOMPARE(parsed.vecPages.at(0).size, QSize(1080, 1440));
    QCOMPARE(parsed.vecPages.at(0).vecComponents.size(), 2);
    QCOMPARE(parsed.vecPages.at(0).vecComponents.at(0).textData.strContent, QStringLiteral("装备名"));
    QCOMPARE(parsed.vecPages.at(0).vecComponents.at(1).eType, E_COMPONENT_TYPE_STICKER);
    QCOMPARE(parsed.vecPages.at(0).vecComponents.at(1).stickerData.eStickerType,
             E_STICKER_TYPE_STAR_RATING);
    QCOMPARE(parsed.vecPages.at(0).vecComponents.at(1).stickerData.color, QColor(200, 50, 50));
}

void TestTemplate::testFileRoundtrip()
{
    QTemporaryDir tempDir;
    Template t;
    t.strName = QStringLiteral("封面");
    t.eType = E_WALKTHROUGH_TYPE_COVER;
    Page page;
    page.strName = QStringLiteral("页面 1");
    page.size = QSize(1080, 1440);
    t.vecPages.append(page);

    const QString strFilePath = tempDir.filePath(QStringLiteral("template.json"));
    QString strErrorMessage;
    QVERIFY2(TemplateSerializer::writeFile(t, strFilePath, &strErrorMessage), qPrintable(strErrorMessage));

    Template parsed;
    QVERIFY2(TemplateSerializer::readFile(strFilePath, &parsed, &strErrorMessage), qPrintable(strErrorMessage));
    QCOMPARE(parsed.strName, QStringLiteral("封面"));
    QCOMPARE(parsed.vecPages.size(), 1);
}

void TestTemplate::testBuiltinTemplates()
{
    const QVector<Template> templates = builtinTemplates();
    QCOMPARE(templates.size(), 6);
    for (const Template& rTemplate : templates) {
        QVERIFY(!rTemplate.strName.isEmpty());
        QVERIFY(!rTemplate.vecPages.isEmpty());
        for (const Page& rPage : rTemplate.vecPages) {
            QVERIFY(rPage.size.width() > 0);
            QVERIFY(rPage.size.height() > 0);
        }
    }
    // 六类类型齐全
    QSet<E_WALKTHROUGH_TYPE> types;
    for (const Template& rTemplate : templates) {
        types.insert(rTemplate.eType);
    }
    QCOMPARE(types.size(), 6);
}

void TestTemplate::testInvalidJson()
{
    Template parsed;
    QString strErrorMessage;
    QVERIFY(!TemplateSerializer::fromJson(QStringLiteral("这不是 JSON {{{"), &parsed, &strErrorMessage));
    QVERIFY(!strErrorMessage.isEmpty());
}

QTEST_GUILESS_MAIN(TestTemplate)

#include "test_template.moc"
