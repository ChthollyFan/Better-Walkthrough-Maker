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
    // 页面含组件时序列化往返一致
    void testComponentRoundtrip();
    // 旧格式（无 components 字段）读取为空组件列表
    void testLegacyFormatWithoutComponents();
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

void TestProjectSerialization::testComponentRoundtrip()
{
    // 构造一个含三种组件的页面
    Component imageComponent;
    imageComponent.strId = QStringLiteral("id-image-1");
    imageComponent.eType = E_COMPONENT_TYPE_IMAGE;
    imageComponent.pos = QPointF(10, 20);
    imageComponent.size = QSizeF(300, 200);
    imageComponent.dRotation = 15;
    imageComponent.nZOrder = 2;
    imageComponent.imageData.strFilePath = QStringLiteral("assets/截图.png");

    Component textComponent;
    textComponent.strId = QStringLiteral("id-text-1");
    textComponent.eType = E_COMPONENT_TYPE_TEXT;
    textComponent.pos = QPointF(50, 60);
    textComponent.textData.strContent = QStringLiteral("出血流装备");
    textComponent.textData.strFontFamily = QStringLiteral("Microsoft YaHei");
    textComponent.textData.nFontSize = 32;
    textComponent.textData.color = QColor(200, 30, 30);
    textComponent.textData.bBold = true;

    Component shapeComponent;
    shapeComponent.strId = QStringLiteral("id-shape-1");
    shapeComponent.eType = E_COMPONENT_TYPE_SHAPE;
    shapeComponent.shapeData.eShapeType = E_SHAPE_TYPE_ROUND_RECT;
    shapeComponent.shapeData.fillColor = QColor(240, 240, 240);
    shapeComponent.shapeData.borderColor = QColor(100, 100, 100);
    shapeComponent.shapeData.nBorderWidth = 2;
    shapeComponent.bLocked = true;

    Project project;
    project.strName = QStringLiteral("组件测试");
    Walkthrough walkthrough;
    walkthrough.strTitle = QStringLiteral("攻略 1");
    Page page;
    page.strName = QStringLiteral("页面 1");
    page.size = QSize(1080, 1440);
    page.vecComponents = {imageComponent, textComponent, shapeComponent};
    walkthrough.vecPages.append(page);
    project.vecWalkthroughs.append(walkthrough);

    const QString strJson = ProjectSerializer::toJson(project);

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage), qPrintable(strErrorMessage));

    const auto& rComponents = parsed.vecWalkthroughs.at(0).vecPages.at(0).vecComponents;
    QCOMPARE(rComponents.size(), 3);

    // 图片组件
    QCOMPARE(rComponents.at(0).strId, QStringLiteral("id-image-1"));
    QCOMPARE(rComponents.at(0).eType, E_COMPONENT_TYPE_IMAGE);
    QCOMPARE(rComponents.at(0).pos, QPointF(10, 20));
    QCOMPARE(rComponents.at(0).size, QSizeF(300, 200));
    QCOMPARE(rComponents.at(0).dRotation, 15.0);
    QCOMPARE(rComponents.at(0).nZOrder, 2);
    QCOMPARE(rComponents.at(0).imageData.strFilePath, QStringLiteral("assets/截图.png"));

    // 文本组件
    QCOMPARE(rComponents.at(1).eType, E_COMPONENT_TYPE_TEXT);
    QCOMPARE(rComponents.at(1).textData.strContent, QStringLiteral("出血流装备"));
    QCOMPARE(rComponents.at(1).textData.nFontSize, 32);
    QCOMPARE(rComponents.at(1).textData.color, QColor(200, 30, 30));
    QVERIFY(rComponents.at(1).textData.bBold);

    // 形状组件
    QCOMPARE(rComponents.at(2).eType, E_COMPONENT_TYPE_SHAPE);
    QCOMPARE(rComponents.at(2).shapeData.eShapeType, E_SHAPE_TYPE_ROUND_RECT);
    QCOMPARE(rComponents.at(2).shapeData.fillColor, QColor(240, 240, 240));
    QCOMPARE(rComponents.at(2).shapeData.nBorderWidth, 2);
    QVERIFY(rComponents.at(2).bLocked);
}

void TestProjectSerialization::testLegacyFormatWithoutComponents()
{
    // M1 旧格式：页面无 components 字段
    const QString strJson = QStringLiteral(R"({
        "formatVersion": 1,
        "name": "旧项目",
        "walkthroughs": [ { "title": "旧攻略", "type": "cover",
            "pages": [ { "name": "旧页面", "width": 1080, "height": 1440 } ] } ]
    })");
    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage), qPrintable(strErrorMessage));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecPages.at(0).strName, QStringLiteral("旧页面"));
    QVERIFY(parsed.vecWalkthroughs.at(0).vecPages.at(0).vecComponents.isEmpty());
}

QTEST_GUILESS_MAIN(TestProjectSerialization)

#include "test_project_serialization.moc"
