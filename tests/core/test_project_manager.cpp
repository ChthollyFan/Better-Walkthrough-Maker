/**
 * @file test_project_manager.cpp
 * @author zhangweimu
 * @brief 项目管理（创建/保存/打开/自动保存/崩溃恢复）的单元测试。
 */
#include <QtTest>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "core/Project.h"
#include "project/ProjectManager.h"

using namespace bwm;

class TestProjectManager : public QObject {
    Q_OBJECT

private slots:
    // 创建 → 目录结构正确 → 打开后数据一致
    void testCreateSaveOpen();
    // 游戏名非法时拒绝创建
    void testCreateInvalidName();
    // 同名项目拒绝创建
    void testCreateDuplicate();
    // 打开损坏文件失败
    void testOpenInvalidJson();
    // 残留 .tmp 快照可恢复
    void testRecoverSnapshot();
    // 自动保存触发后写入文件并清除脏标记
    void testAutoSave();
};

void TestProjectManager::testCreateSaveOpen()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    ProjectManager manager;
    QString strErrorMessage;
    QVERIFY2(manager.createProject(QStringLiteral("艾尔登法环"), QSize(1080, 1440),
                                   tempDir.path(), &strErrorMessage),
             qPrintable(strErrorMessage));

    const QString strProjectDir = tempDir.filePath(QStringLiteral("艾尔登法环.bwm"));
    QVERIFY(QDir(strProjectDir).exists());
    QVERIFY(QDir(strProjectDir).exists(QStringLiteral("assets")));
    QVERIFY(QFile::exists(strProjectDir + QStringLiteral("/project.json")));

    // 新建项目自带一个攻略与一个页面
    QCOMPARE(manager.project()->strName, QStringLiteral("艾尔登法环"));
    QCOMPARE(manager.project()->vecWalkthroughs.size(), 1);
    QCOMPARE(manager.project()->vecWalkthroughs.at(0).vecPages.size(), 1);
    QCOMPARE(manager.project()->vecWalkthroughs.at(0).vecPages.at(0).size, QSize(1080, 1440));

    // 用新的管理器打开同一项目，数据一致
    ProjectManager opener;
    const QString strJsonPath = strProjectDir + QStringLiteral("/project.json");
    QVERIFY2(opener.openProject(strJsonPath, &strErrorMessage), qPrintable(strErrorMessage));
    QCOMPARE(opener.project()->strName, QStringLiteral("艾尔登法环"));
    QCOMPARE(opener.project()->vecWalkthroughs.at(0).vecPages.at(0).size, QSize(1080, 1440));
}

void TestProjectManager::testCreateInvalidName()
{
    QTemporaryDir tempDir;
    ProjectManager manager;
    QString strErrorMessage;

    QVERIFY(!manager.createProject(QString(), QSize(1080, 1440), tempDir.path(), &strErrorMessage));
    QVERIFY(!strErrorMessage.isEmpty());
    QVERIFY(!manager.createProject(QStringLiteral("   "), QSize(1080, 1440), tempDir.path(), &strErrorMessage));

    // 父目录不存在
    QVERIFY(!manager.createProject(QStringLiteral("游戏"), QSize(1080, 1440),
                                   tempDir.path() + QStringLiteral("/不存在"), &strErrorMessage));
}

void TestProjectManager::testCreateDuplicate()
{
    QTemporaryDir tempDir;
    ProjectManager manager;
    QString strErrorMessage;

    QVERIFY2(manager.createProject(QStringLiteral("重复测试"), QSize(1080, 1440),
                                   tempDir.path(), &strErrorMessage),
             qPrintable(strErrorMessage));
    QVERIFY(!manager.createProject(QStringLiteral("重复测试"), QSize(1080, 1440),
                                   tempDir.path(), &strErrorMessage));
}

void TestProjectManager::testOpenInvalidJson()
{
    QTemporaryDir tempDir;
    const QString strJsonPath = tempDir.filePath(QStringLiteral("project.json"));
    QFile file(strJsonPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{{{ 损坏的 JSON");
    file.close();

    ProjectManager manager;
    QString strErrorMessage;
    QVERIFY(!manager.openProject(strJsonPath, &strErrorMessage));
    QVERIFY(!strErrorMessage.isEmpty());
}

void TestProjectManager::testRecoverSnapshot()
{
    QTemporaryDir tempDir;
    ProjectManager manager;
    QString strErrorMessage;
    QVERIFY2(manager.createProject(QStringLiteral("恢复测试"), QSize(1080, 1440),
                                   tempDir.path(), &strErrorMessage),
             qPrintable(strErrorMessage));

    const QString strProjectDir = tempDir.filePath(QStringLiteral("恢复测试.bwm"));
    const QString strJsonPath = strProjectDir + QStringLiteral("/project.json");
    const QString strTmpPath = strJsonPath + QStringLiteral(".tmp");

    // 模拟一次未完成的保存：写入内容更新的 .tmp 残留
    QFile tmpFile(strTmpPath);
    QVERIFY(tmpFile.open(QIODevice::WriteOnly));
    tmpFile.write(R"({"formatVersion":1,"name":"恢复后的名字"})");
    QVERIFY(tmpFile.setFileTime(QDateTime::currentDateTime().addSecs(60),
                                QFileDevice::FileModificationTime));
    tmpFile.close();

    QVERIFY(ProjectManager::hasRecoverableSnapshot(strProjectDir));
    QVERIFY2(manager.recoverFromSnapshot(&strErrorMessage), qPrintable(strErrorMessage));
    QVERIFY(!QFile::exists(strTmpPath));

    // 重新打开，应读到恢复后的内容
    ProjectManager opener;
    QVERIFY2(opener.openProject(strJsonPath, &strErrorMessage), qPrintable(strErrorMessage));
    QCOMPARE(opener.project()->strName, QStringLiteral("恢复后的名字"));
}

void TestProjectManager::testAutoSave()
{
    QTemporaryDir tempDir;
    ProjectManager manager;
    QString strErrorMessage;
    QVERIFY2(manager.createProject(QStringLiteral("自动保存"), QSize(1080, 1440),
                                   tempDir.path(), &strErrorMessage),
             qPrintable(strErrorMessage));

    manager.setAutoSaveIntervalMs(100);
    manager.setDirty();
    const QDateTime beforeSave = QFileInfo(
        tempDir.filePath(QStringLiteral("自动保存.bwm/project.json"))).lastModified();

    QSignalSpy spy(&manager, &ProjectManager::autoSavePerformed);
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 3000);

    QVERIFY(spy.at(0).at(0).toBool()); // 保存成功
    QVERIFY(!manager.isDirty());

    const QDateTime afterSave = QFileInfo(
        tempDir.filePath(QStringLiteral("自动保存.bwm/project.json"))).lastModified();
    QVERIFY(afterSave >= beforeSave);
}

QTEST_GUILESS_MAIN(TestProjectManager)

#include "test_project_manager.moc"
