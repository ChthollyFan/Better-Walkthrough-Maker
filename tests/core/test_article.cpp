/**
 * @file test_article.cpp
 * @author zhangweimu
 * @brief 文章攻略（Article）数据模型与序列化的单元测试。
 *
 * 文章攻略嵌套在 Walkthrough 内（与页面同级），不再独立存在于 Project 层。
 * 注意：本文件不使用 C++ raw string literal，避免 moc 对 raw string 的已知解析问题。
 */
#include <QtTest>

#include "core/Article.h"
#include "core/Project.h"
#include "core/ProjectSerializer.h"

using namespace bwm;

class TestArticle : public QObject {
    Q_OBJECT

private slots:
    void testRoundtrip();
    void testPageRefRoundtrip();
    void testLegacyWithoutArticles();
    void testLegacyProjectLevelArticlesMigration();
    void testMixedPageAndArticle();
    void testMissingFields();
    void testCorruptArticleEntries();
    void testMakePageRef();
    void testContainsPageRef();
};

void TestArticle::testRoundtrip()
{
    Project project;
    project.strName = QStringLiteral("测试游戏");

    Walkthrough walkthrough;
    walkthrough.strTitle = QStringLiteral("新手攻略");
    walkthrough.eType = E_WALKTHROUGH_TYPE_EQUIPMENT;

    Article article;
    article.strId = QStringLiteral("art-001");
    article.strTitle = QStringLiteral("新手入门指南");
    article.strMarkdown = QStringLiteral(
        "# 新手入门\n\n"
        "## 武器推荐\n\n"
        "- 猎犬长牙：前期强力武器\n"
        "- 名刀月隐：魔法战士首选\n\n"
        "![装备图](assets/weapon.png)\n");
    walkthrough.vecArticles.append(article);
    project.vecWalkthroughs.append(walkthrough);

    const QString strJson = ProjectSerializer::toJson(project);

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    QCOMPARE(parsed.vecWalkthroughs.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.size(), 1);
    const Article& rParsed = parsed.vecWalkthroughs.at(0).vecArticles.at(0);
    QCOMPARE(rParsed.strId, QStringLiteral("art-001"));
    QCOMPARE(rParsed.strTitle, QStringLiteral("新手入门指南"));
    QVERIFY(rParsed.strMarkdown.contains(QStringLiteral("猎犬长牙")));
    QVERIFY(rParsed.strMarkdown.contains(QStringLiteral("![装备图](assets/weapon.png)")));
}

void TestArticle::testPageRefRoundtrip()
{
    const QString strMarkdownWithRef = QStringLiteral(
        "参见装备详情页：![[0:1]]，以及武器对比页 ![[0:2]]\n");

    Project project;
    project.strName = QStringLiteral("页面引用测试");

    Walkthrough walkthrough;
    walkthrough.strTitle = QStringLiteral("攻略1");

    Article article;
    article.strId = QStringLiteral("art-ref");
    article.strTitle = QStringLiteral("引用测试");
    article.strMarkdown = strMarkdownWithRef;
    walkthrough.vecArticles.append(article);
    project.vecWalkthroughs.append(walkthrough);

    const QString strJson = ProjectSerializer::toJson(project);

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strMarkdown, strMarkdownWithRef);
    QVERIFY(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strMarkdown.contains(QStringLiteral("![[0:1]]")));
    QVERIFY(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strMarkdown.contains(QStringLiteral("![[0:2]]")));
}

void TestArticle::testLegacyWithoutArticles()
{
    // 旧格式文件：攻略无 articles 字段（用 QStringLiteral 拼接避免 raw string）
    const QString strJson = QStringLiteral(
        "{ \"formatVersion\": 1, \"name\": \"旧项目\", "
        "\"walkthroughs\": [ { \"title\": \"图文攻略\", \"type\": \"cover\", "
        "\"pages\": [ { \"name\": \"页面1\", \"width\": 1080, \"height\": 1440 } ] } ] }");

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    QCOMPARE(parsed.vecWalkthroughs.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).strTitle, QStringLiteral("图文攻略"));
    QVERIFY(parsed.vecWalkthroughs.at(0).vecArticles.isEmpty());
}

void TestArticle::testLegacyProjectLevelArticlesMigration()
{
    // 旧版本文件：articles 在 project 级，反序列化时应迁移到第一个攻略内
    const QString strJson = QStringLiteral(
        "{ \"formatVersion\": 1, \"name\": \"旧版本项目\", "
        "\"walkthroughs\": [ { \"title\": \"已有攻略\", \"type\": \"cover\" } ], "
        "\"articles\": [ { \"id\": \"old-art\", \"title\": \"旧文章\", \"markdown\": \"# 旧内容\" } ] }");

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    // 迁移后：文章应在第一个攻略内
    QCOMPARE(parsed.vecWalkthroughs.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strId, QStringLiteral("old-art"));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strTitle, QStringLiteral("旧文章"));
}

void TestArticle::testMixedPageAndArticle()
{
    Project project;
    project.strName = QStringLiteral("混合项目");

    Walkthrough walkthrough;
    walkthrough.strTitle = QStringLiteral("装备攻略");
    walkthrough.eType = E_WALKTHROUGH_TYPE_EQUIPMENT;

    Page page;
    page.strName = QStringLiteral("装备总览");
    page.size = QSize(1080, 1440);
    walkthrough.vecPages.append(page);

    Article article;
    article.strId = QStringLiteral("art-1");
    article.strTitle = QStringLiteral("装备文章");
    article.strMarkdown = QStringLiteral("图文攻略见 ![[0:0]]");
    walkthrough.vecArticles.append(article);

    project.vecWalkthroughs.append(walkthrough);

    const QString strJson = ProjectSerializer::toJson(project);

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    QCOMPARE(parsed.vecWalkthroughs.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecPages.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecPages.at(0).strName, QStringLiteral("装备总览"));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strTitle, QStringLiteral("装备文章"));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strMarkdown, QStringLiteral("图文攻略见 ![[0:0]]"));
}

void TestArticle::testMissingFields()
{
    // 攻略的 articles 数组中元素缺失字段时取默认值
    const QString strJson = QStringLiteral(
        "{ \"name\": \"缺字段测试\", "
        "\"walkthroughs\": [ { \"title\": \"攻略1\", \"articles\": [ {} ] } ] }");

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.size(), 1);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strId, QString());
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strTitle, QStringLiteral("未命名文章"));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strMarkdown, QString());
}

void TestArticle::testCorruptArticleEntries()
{
    // articles 中混入非对象条目，应跳过损坏项，保留合法项
    const QString strJson = QStringLiteral(
        "{ \"walkthroughs\": [ { \"title\": \"攻略1\", \"articles\": [ "
        "\"这不是对象\", 42, "
        "{ \"id\": \"good\", \"title\": \"合法文章\", \"markdown\": \"# 正文\" }, "
        "null, "
        "{ \"id\": \"good2\", \"title\": \"第二篇\", \"markdown\": \"正文2\" } "
        "] } ] }");

    Project parsed;
    QString strErrorMessage;
    QVERIFY2(ProjectSerializer::fromJson(strJson, &parsed, &strErrorMessage),
             qPrintable(strErrorMessage));

    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.size(), 2);
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.at(0).strId, QStringLiteral("good"));
    QCOMPARE(parsed.vecWalkthroughs.at(0).vecArticles.at(1).strId, QStringLiteral("good2"));
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

    QVERIFY(!containsPageRef(QStringLiteral("![图片](assets/x.png)")));
    QVERIFY(!containsPageRef(QStringLiteral("普通文本无引用")));
    QVERIFY(!containsPageRef(QString()));

    QVERIFY(!containsPageRef(QStringLiteral("![[0]]")));
    QVERIFY(!containsPageRef(QStringLiteral("![[0:]]")));
    QVERIFY(!containsPageRef(QStringLiteral("[[0:1]]")));
}

QTEST_GUILESS_MAIN(TestArticle)

#include "test_article.moc"
