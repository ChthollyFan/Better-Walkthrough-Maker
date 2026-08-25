// 数据模型与 project.json 序列化的单元测试（tests/ 镜像 src/core/）
#include <QtTest>

#include "core/Project.h"
#include "core/ProjectSerializer.h"

using namespace bwm;

class TestProjectSerialization : public QObject {
    Q_OBJECT

private slots:
    // 完整项目序列化往返后字段一致
    void testRoundtrip();
    // 缺失字段取默认值（{}、缺 walkthroughs、缺 title/type/pages、缺尺寸）
    void testMissingFields();
    // 未知攻略类型回退为 Custom
    void testUnknownType();
    // 非法 JSON 解析失败并给出原因
    void testBadJson();
    // 高于当前格式版本的文件拒绝打开
    void testFutureVersion();
    // 非法页面尺寸（零、负数、超上限）回退默认尺寸
    void testInvalidPageSize();
};

void TestProjectSerialization::testRoundtrip()
{
    Project project;
    project.name = QStringLiteral("艾尔登法环");

    Walkthrough equipment;
    equipment.title = QStringLiteral("出血流装备推荐");
    equipment.type = WalkthroughType::Equipment;
    equipment.pages = {
        {QStringLiteral("装备总览"), QSize(1080, 1440)},
        {QStringLiteral("武器对比"), QSize(1920, 1080)},
    };
    Walkthrough story;
    story.title = QStringLiteral("主线剧情流程");
    story.type = WalkthroughType::StoryFlow;
    story.pages = {{QStringLiteral("第一章"), QSize(1080, 1440)}};
    project.walkthroughs = {equipment, story};

    const QString json = ProjectSerializer::toJson(project);

    Project parsed;
    QString errorMessage;
    QVERIFY2(ProjectSerializer::fromJson(json, &parsed, &errorMessage),
             qPrintable(errorMessage));

    QCOMPARE(parsed.name, project.name);
    QCOMPARE(parsed.walkthroughs.size(), 2);

    QCOMPARE(parsed.walkthroughs.at(0).title, QStringLiteral("出血流装备推荐"));
    QCOMPARE(parsed.walkthroughs.at(0).type, WalkthroughType::Equipment);
    QCOMPARE(parsed.walkthroughs.at(0).pages.size(), 2);
    QCOMPARE(parsed.walkthroughs.at(0).pages.at(0).name, QStringLiteral("装备总览"));
    QCOMPARE(parsed.walkthroughs.at(0).pages.at(0).size, QSize(1080, 1440));
    QCOMPARE(parsed.walkthroughs.at(0).pages.at(1).size, QSize(1920, 1080));

    QCOMPARE(parsed.walkthroughs.at(1).type, WalkthroughType::StoryFlow);
    QCOMPARE(parsed.walkthroughs.at(1).pages.at(0).size, QSize(1080, 1440));
}

void TestProjectSerialization::testMissingFields()
{
    Project parsed;
    QString errorMessage;

    // 空对象：全部取默认值
    QVERIFY2(ProjectSerializer::fromJson(QStringLiteral("{}"), &parsed, &errorMessage),
             qPrintable(errorMessage));
    QCOMPARE(parsed.name, QString());
    QVERIFY(parsed.walkthroughs.isEmpty());

    // 只有 name，无 walkthroughs
    QVERIFY2(ProjectSerializer::fromJson(QStringLiteral(R"({"name":"测试"})"), &parsed, &errorMessage),
             qPrintable(errorMessage));
    QCOMPARE(parsed.name, QStringLiteral("测试"));
    QVERIFY(parsed.walkthroughs.isEmpty());

    // 攻略缺 title/type/pages
    const QString json = QStringLiteral(R"({
        "formatVersion": 1,
        "name": "测试",
        "walkthroughs": [ {} ]
    })");
    QVERIFY2(ProjectSerializer::fromJson(json, &parsed, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(parsed.walkthroughs.size(), 1);
    QCOMPARE(parsed.walkthroughs.at(0).title, QStringLiteral("未命名攻略"));
    QCOMPARE(parsed.walkthroughs.at(0).type, WalkthroughType::Cover);
    QVERIFY(parsed.walkthroughs.at(0).pages.isEmpty());

    // 页面缺名称与尺寸
    const QString json2 = QStringLiteral(R"({
        "walkthroughs": [ { "pages": [ {} ] } ]
    })");
    QVERIFY2(ProjectSerializer::fromJson(json2, &parsed, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(parsed.walkthroughs.at(0).pages.at(0).name, QStringLiteral("未命名页面"));
    QCOMPARE(parsed.walkthroughs.at(0).pages.at(0).size, QSize(1080, 1440));
}

void TestProjectSerialization::testUnknownType()
{
    Project parsed;
    QString errorMessage;
    const QString json = QStringLiteral(R"({
        "walkthroughs": [ { "type": "unknown_thing" } ]
    })");
    QVERIFY2(ProjectSerializer::fromJson(json, &parsed, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(parsed.walkthroughs.at(0).type, WalkthroughType::Custom);

    // 非字符串 type 也回退为 Cover
    const QString json2 = QStringLiteral(R"({
        "walkthroughs": [ { "type": 42 } ]
    })");
    QVERIFY2(ProjectSerializer::fromJson(json2, &parsed, &errorMessage), qPrintable(errorMessage));
    QCOMPARE(parsed.walkthroughs.at(0).type, WalkthroughType::Cover);
}

void TestProjectSerialization::testBadJson()
{
    Project parsed;
    QString errorMessage;
    QVERIFY(!ProjectSerializer::fromJson(QStringLiteral("这不是 JSON {{{"), &parsed, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    // 根是数组而非对象
    QVERIFY(!ProjectSerializer::fromJson(QStringLiteral("[1,2,3]"), &parsed, &errorMessage));
}

void TestProjectSerialization::testFutureVersion()
{
    Project parsed;
    QString errorMessage;
    const QString json = QStringLiteral(R"({ "formatVersion": 999, "name": "未来" })");
    QVERIFY(!ProjectSerializer::fromJson(json, &parsed, &errorMessage));
    QVERIFY(errorMessage.contains(QStringLiteral("更新版本")));
}

void TestProjectSerialization::testInvalidPageSize()
{
    Project parsed;
    QString errorMessage;
    // 零、负数、超上限均回退默认尺寸
    const QString json = QStringLiteral(R"({
        "walkthroughs": [ { "pages": [
            { "width": 0, "height": 100 },
            { "width": -5, "height": 100 },
            { "width": 99999, "height": 100 }
        ] } ]
    })");
    QVERIFY2(ProjectSerializer::fromJson(json, &parsed, &errorMessage), qPrintable(errorMessage));
    const auto &pages = parsed.walkthroughs.at(0).pages;
    QCOMPARE(pages.size(), 3);
    for (const Page &page : pages)
        QCOMPARE(page.size, QSize(1080, 1440));
}

QTEST_GUILESS_MAIN(TestProjectSerialization)

#include "test_project_serialization.moc"
