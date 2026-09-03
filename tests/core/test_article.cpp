/**
 * @file test_article.cpp
 * @author zhangweimu
 * @brief 文章攻略（Article）数据模型与序列化的单元测试。
 *
 * 覆盖：
 * - Article 序列化往返（id / title / markdown 完整保留）
 * - 含页面引用语法的 Markdown 正文往返
 * - 向后兼容：旧文件无 articles 字段时解析为空列表
 * - 向后兼容：旧版本软件忽略 articles 字段（通过未知字段容错保证）
 * - 混合项目：图文攻略与文章攻略共存
 * - makePageRef / containsPageRef 工具函数
 * - 缺失字段取默认值
 */
#include <QtTest>

#include "core/Article.h"
#include "core/Project.h"
#include "core/ProjectSerializer.h"

using namespace bwm;

class TestArticle : public QObject {
    Q_OBJECT

private slots:
    // Article 完整序列化往返
    void testRoundtrip();
    // Markdown 正文中含页面引用标记 ![[W:P]] 时往返保留
    void testPageRefRoundtrip();
    // 旧文件无 articles 字段时解析为空（向后兼容）
    void testLegacyWithoutArticles();
    // 图文攻略与文章攻略共存
    void testMixedWalkthroughAndArticle();
    // 文章缺失字段取默认值
    void testMissingFields();
    // 损坏的 articles 条目（非对象）被跳过，不影响其余条目
    void testCorruptArticleEntries();
    // makePageRef 生成格式正确
    void testMakePageRef();
    // containsPageRef 检测页面引用
    void testContainsPageRef();
};

void TestArticle::testRoundtrip()
{
    Project project;
    project.strName = QStringLiteral("测试游戏");

    Article article;
    article.strId = QStringLiteral("art-001");
    article.strTitle = QStringLiteral("新手入门指南");
    article.strMarkdown = QStringLiteral(
        "# 新手入门\n\n"
        "## 武器推荐\n\n"
        "- 猎犬长牙：前期强力武器\n"
        "- 名刀月隐：魔法战士首选\n\n"
        "![装备图](assets/weapon.png)\n");
    project.vecArticles.append(article);

    const QString strJson = ProjectSerializer::toJson(project);

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    QCOMPARE(parsed.vecArticles.size(), 1);
    const Article& rParsed = parsed.vecArticles.at(0);
    QCOMPARE(rParsed.strId, QStringLiteral("art-001"));
    QCOMPARE(rParsed.strTitle, QStringLiteral("新手入门指南"));
    QVERIFY(rParsed.strMarkdown.contains(QStringLiteral("猎犬长牙")));
    QVERIFY(rParsed.strMarkdown.contains(QStringLiteral("![装备图](assets/weapon.png)")));
}

void TestArticle::testPageRefRoundtrip()
{
    // Markdown 正文中含页面引用标记，序列化后必须原样保留
    const QString strMarkdownWithRef = QStringLiteral(
        "参见装备详情页：![[0:1]]，以及武器对比页 ![[0:2]]\n");

    Project project;
    project.strName = QStringLiteral("页面引用测试");

    Article article;
    article.strId = QStringLiteral("art-ref");
    article.strTitle = QStringLiteral("引用测试");
    article.strMarkdown = strMarkdownWithRef;
    project.vecArticles.append(article);

    const QString strJson = ProjectSerializer::toJson(project);

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    QCOMPARE(parsed.vecArticles.size(), 1);
    // 页面引用标记原样保留
    QCOMPARE(parsed.vecArticles.at(0).strMarkdown, strMarkdownWithRef);
    QVERIFY(parsed.vecArticles.at(0).strMarkdown.contains(QStringLiteral("![[0:1]]")));
    QVERIFY(parsed.vecArticles.at(0).strMarkdown.contains(QStringLiteral("![[0:2]]")));
}

void TestArticle::testLegacyWithoutArticles()
{
    // 旧格式文件：只有 walkthroughs，无 articles 字段
    const QString strJson = QStringLiteral(R"({
        "formatVersion": 1,
        "name": "旧项目",
        "walkthroughs": [ { "title": "图文攻略", "type": "cover",
            "pages": [ { "name": "页面1", "width": 1080, "height": 1440 } ] } ]
    })");

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    // 图文攻略正常解析
    QCOMPARE(parsed.vecWalkthroughs.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).strTitle, QStringLiteral("图文攻略"));
    // 文章列表为空（向后兼容）
    QVERIFY(parsed.vecArticles.isEmpty());
}

void TestArticle::testMixedWalkthroughAndArticle()
{
    Project project;
    project.strName = QStringLiteral("混合项目");

    // 图文攻略
    Walkthrough walkthrough;
    walkthrough.strTitle = QStringLiteral("装备图文");
    walkthrough.eType = E_WALKTHROUGH_TYPE_EQUIPMENT;
    Page page;
    page.strName = QStringLiteral("装备总览");
    page.size = QSize(1080, 1440);
    walkthrough.vecPages.append(page);
    project.vecWalkthroughs.append(walkthrough);

    // 文章攻略
    Article article;
    article.strId = QStringLiteral("art-1");
    article.strTitle = QStringLiteral("装备文章");
    article.strMarkdown = QStringLiteral("图文攻略见 ![[0:0]]");
    project.vecArticles.append(article);

    const QString strJson = ProjectSerializer::toJson(project);

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    // 两类攻略都正确解析
    QCOMPARE(parsed.vecWalkthroughs.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).strTitle, QStringLiteral("装备图文"));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecPages.at(0).strName, QStringLiteral("装备总览"));

    QCOMPARE(parsed.vecArticles.size(), 1);
    QCOMPARE(parsed.vecArticles.at(0).strTitle, QStringLiteral("装备文章"));
    QCOMPARE(parsed.vecArticles.at(0).strMarkdown, QStringLiteral("图文攻略见 ![[0:0]]"));
}

void TestArticle::testMissingFields()
{
    // articles 数组中元素缺失字段时取默认值
    const QString strJson = QStringLiteral(R"({
        "name": "缺字段测试",
        "articles": [ {} ]
    })");

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    QCOMPARE(parsed.vecArticles.size(), 1);
    QCOMPARE(parsed.vecArticles.at(0).strId, QString());       // 缺失为空
    QCOMPARE(parsed.vecArticles.at(0).strTitle, QStringLiteral("未命名文章"));  // 默认标题
    QCOMPARE(parsed.vecArticles.at(0).strMarkdown, QString()); // 缺失为空
}

void TestArticle::testCorruptArticleEntries()
{
    // articles 中混入非对象条目，应跳过损坏项，保留合法项
    const QString strJson = QStringLiteral(R"({
        "articles": [
            "这不是对象",
            42,
            { "id": "good", "title": "合法文章", "markdown": "# 正文" },
            null,
            { "id": "good2", "title": "第二篇", "markdown": "正文2" }
        ]
    })");

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    // 只保留两个合法对象条目
    QCOMPARE(parsed.vecArticles.size(), 2);
    QCOMPARE(parsed.vecArticles.at(0).strId, QStringLiteral("good"));
    QCOMPARE(parsed.vecArticles.at(1).strId, QStringLiteral("good2"));
}

void TestArticle::testMakePageRef()
{
    QCOMPARE(makePageRef(0, 1), QStringLiteral("![[0:1]]"));
    QCOMPARE(makePageRef(3, 5), QStringLiteral("![[3:5]]"));
    QCOMPARE(makePageRef(0, 0), QStringLiteral("![[0:0]]"));
}

void TestArticle::testContainsPageRef()
{
    QVERIFY(containsPageRef(QStringLiteral("参见 ![[0:1]] 页面")));
    QVERIFY(containsPageRef(QStringLiteral("![[3:5]]")));
    QVERIFY(containsPageRef(QStringLiteral("多个引用 ![[0:1]] 和 ![[2:3]]")));

    // 普通图片语法不应被识别为页面引用
    QVERIFY(!containsPageRef(QStringLiteral("![图片](assets/x.png)")));
    QVERIFY(!containsPageRef(QStringLiteral("普通文本无引用")));
    QVERIFY(!containsPageRef(QString()));

    // 格式不完整的引用不被识别
    QVERIFY(!containsPageRef(QStringLiteral("![[0]]")));      // 缺少冒号和页码
    QVERIFY(!containsPageRef(QStringLiteral("![[0:]]")));     // 缺少页码
    QVERIFY(!containsPageRef(QStringLiteral("[[0:1]]")));     // 缺少感叹号
}

QTEST_GUILESS_MAIN(TestArticle)

#include "test_article.moc"
