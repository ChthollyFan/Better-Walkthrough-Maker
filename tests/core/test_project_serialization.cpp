/**
 * @file test_project_serialization.cpp
 * @author zhangweimu
 * @brief 数据模型与 project.json 序列化的单元测试（tests/ 镜像 src/core/）。
 */
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
    project.strName = QStringLiteral("艾尔登法环");

    Walkthrough equipment;
    equipment.strTitle = QStringLiteral("出血流装备推荐");
    equipment.eType = E_WALKTHROUGH_TYPE_EQUIPMENT;
    equipment.vecPages = {
        {QStringLiteral("装备总览"), QSize(1080, 1440)},
        {QStringLiteral("武器对比"), QSize(1920, 1080)},
    };
    Walkthrough story;
    story.strTitle = QStringLiteral("主线剧情流程");
    story.eType = E_WALKTHROUGH_TYPE_STORY_FLOW;
    story.vecPages = {{QStringLiteral("第一章"), QSize(1080, 1440)}};
    project.vecWalkthroughs = {equipment, story};

    const QString strJson = ProjectSerializer::toJson(project);

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    QCOMPARE(parsed.strName, project.strName);
    QCOMPARE(parsed.vecWalkthroughs.size(), 2);

    QCOMPARE(parsed.vecWalkthroughs.at(0).strTitle, QStringLiteral("出血流装备推荐"));
    QCOMPARE(parsed.vecWalkthroughs.at(0).eType, E_WALKTHROUGH_TYPE_EQUIPMENT);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecPages.size(), 2);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecPages.at(0).strName, QStringLiteral("装备总览"));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecPages.at(0).size, QSize(1080, 1440));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecPages.at(1).size, QSize(1920, 1080));

    QCOMPARE(parsed.vecWalkthroughs.at(1).eType, E_WALKTHROUGH_TYPE_STORY_FLOW);
    QCOMPARE(parsed.vecWalkthroughs.at(1).vecPages.at(0).size, QSize(1080, 1440));
}

void TestProjectSerialization::testMissingFields()
{
    Project parsed;
    QString strErrorMessage;

    // 空对象：全部取默认值
    QVERIFY2(ProjectSerializer::fromJson(QStringLiteral("{}"), &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));
    QCOMPARE(parsed.strName, QString());
    QVERIFY(parsed.vecWalkthroughs.isEmpty());

    // 只有 name，无 walkthroughs
    QVERIFY2(ProjectSerializer::fromJson(QStringLiteral(R"({"name":"测试"})"), &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));
    QCOMPARE(parsed.strName, QStringLiteral("测试"));
    QVERIFY(parsed.vecWalkthroughs.isEmpty());

    // 攻略缺 title/type/pages
    const QString strJson = QStringLiteral(R"({
        "formatVersion": 1,
        "name": "测试",
        "walkthroughs": [ {} ]
    })");
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage), qPrintable(strErrorMessage));
    QCOMPARE(parsed.vecWalkthroughs.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).strTitle, QStringLiteral("未命名攻略"));
    QCOMPARE(parsed.vecWalkthroughs.at(0).eType, E_WALKTHROUGH_TYPE_COVER);
    QVERIFY(parsed.vecWalkthroughs.at(0).vecPages.isEmpty());

    // 页面缺名称与尺寸
    const QString strJson2 = QStringLiteral(R"({
        "walkthroughs": [ { "pages": [ {} ] } ]
    })");
    QVERIFY2(ProjectSerializer::fromJson(strJson2, &parsed, &strErrorMessage), qPrintable(strErrorMessage));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecPages.at(0).strName, QStringLiteral("未命名页面"));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecPages.at(0).size, QSize(1080, 1440));
}

void TestProjectSerialization::testUnknownType()
{
    Project parsed;
    QString strErrorMessage;
    const QString strJson = QStringLiteral(R"({
        "walkthroughs": [ { "type": "unknown_thing" } ]
    })");
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage), qPrintable(strErrorMessage));
    QCOMPARE(parsed.vecWalkthroughs.at(0).eType, E_WALKTHROUGH_TYPE_CUSTOM);

    // 非字符串 type 也回退为 Cover
    const QString strJson2 = QStringLiteral(R"({
        "walkthroughs": [ { "type": 42 } ]
    })");
    QVERIFY2(ProjectSerializer::fromJson(strJson2, &parsed, &strErrorMessage), qPrintable(strErrorMessage));
    QCOMPARE(parsed.vecWalkthroughs.at(0).eType, E_WALKTHROUGH_TYPE_COVER);
}

void TestProjectSerialization::testBadJson()
{
    Project parsed;
    QString strErrorMessage;
    QVERIFY(!ProjectSerializer::fromJson(QStringLiteral("这不是 JSON {{{"), &parsed, &strErrorMessage));
    QVERIFY(!strErrorMessage.isEmpty());

    // 根是数组而非对象
    QVERIFY(!ProjectSerializer::fromJson(QStringLiteral("[1,2,3]"), &parsed, &strErrorMessage));
}

void TestProjectSerialization::testFutureVersion()
{
    Project parsed;
    QString strErrorMessage;
    const QString strJson = QStringLiteral(R"({ "formatVersion": 999, "name": "未来" })");
    QVERIFY(!ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage));
    QVERIFY(strErrorMessage.contains(QStringLiteral("更新版本")));
}

void TestProjectSerialization::testInvalidPageSize()
{
    Project parsed;
    QString strErrorMessage;
    // 零、负数、超上限均回退默认尺寸
    const QString strJson = QStringLiteral(R"({
        "walkthroughs": [ { "pages": [
            { "width": 0, "height": 100 },
            { "width": -5, "height": 100 },
            { "width": 99999, "height": 100 }
        ] } ]
    })");
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage), qPrintable(strErrorMessage));
    const auto& rPages = parsed.vecWalkthroughs.at(0).vecPages;
    QCOMPARE(rPages.size(), 3);
    for (const Page& rPage : rPages) {
        QCOMPARE(rPage.size, QSize(1080, 1440));
    }
}

QTEST_GUILESS_MAIN(TestProjectSerialization)

#include "test_project_serialization.moc"
