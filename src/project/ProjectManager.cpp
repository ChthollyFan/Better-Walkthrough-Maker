/**
 * @file ProjectManager.cpp
 * @author zhangweimu
 * @brief 项目管理实现：新建 / 打开 / 保存 / 自动保存 / 崩溃恢复。
 */
#include "project/ProjectManager.h"

#include "core/ProjectSerializer.h"
#include "settings/Settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace bwm {

namespace {

constexpr int nMaxRecentProjects = 10;

// 将项目路径加入最近项目列表（内部辅助函数）。
void addRecentProject(const QString& strJsonPath)
{
    QStringList recents = Settings::recentProjects();
    recents.removeAll(strJsonPath);
    recents.prepend(strJsonPath);
    while (recents.size() > nMaxRecentProjects) {
        recents.removeLast();
    }
    Settings::setRecentProjects(recents);
}

} // namespace

ProjectManager::ProjectManager(QObject* pParent)
    : QObject(pParent)
{
    m_autoSaveTimer.setSingleShot(false);
    connect(&m_autoSaveTimer, &QTimer::timeout, this, &ProjectManager::onAutoSaveTimeout);
}

bool ProjectManager::hasProject() const
{
    return !m_project.strFilePath.isEmpty();
}

Project* ProjectManager::project()
{
    return &m_project;
}

const Project* ProjectManager::project() const
{
    return &m_project;
}

QString ProjectManager::projectDirectory() const
{
    return QFileInfo(m_project.strFilePath).absolutePath();
}

bool ProjectManager::isDirty() const
{
    return m_bDirty;
}

bool ProjectManager::createProject(const QString& strGameName, const QSize& size,
                                   const QString& strParentDirectory, QString* pErrorMessage)
{
    if (strGameName.trimmed().isEmpty()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("游戏名不能为空");
        }
        return false;
    }

    const QDir parentDir(strParentDirectory);
    if (!parentDir.exists()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("目录不存在：%1").arg(strParentDirectory);
        }
        return false;
    }

    const QString strProjectDirName = strGameName.trimmed() + QStringLiteral(".bwm");
    const QString strProjectDirPath = parentDir.filePath(strProjectDirName);
    QDir projectDir(strProjectDirPath);
    if (projectDir.exists()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("项目已存在：%1").arg(strProjectDirPath);
        }
        return false;
    }

    if (!projectDir.mkpath(strProjectDirPath) || !projectDir.mkpath(QStringLiteral("assets"))) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("无法创建项目目录：%1").arg(strProjectDirPath);
        }
        return false;
    }

    m_project = Project();
    m_project.strName = strGameName.trimmed();
    m_project.strFilePath = projectJsonPath(strProjectDirPath);

    // 首个攻略与页面：让新建的项目立刻可用（名称与类型可后续修改）
    Walkthrough initialWalkthrough;
    initialWalkthrough.strTitle = QStringLiteral("攻略 1");
    initialWalkthrough.eType = E_WALKTHROUGH_TYPE_COVER;
    Page initialPage;
    initialPage.strName = QStringLiteral("页面 1");
    initialPage.size = size.isValid() && size.width() > 0 && size.height() > 0
        ? size
        : Settings::defaultPageSize();
    initialWalkthrough.vecPages.append(initialPage);
    m_project.vecWalkthroughs.append(initialWalkthrough);

    m_bDirty = true;
    if (!save(pErrorMessage)) {
        return false;
    }

    startAutoSaveTimer();
    emit projectOpened();
    return true;
}

bool ProjectManager::openProject(const QString& strJsonPath, QString* pErrorMessage)
{
    QFile file(strJsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("无法打开文件：%1（%2）").arg(strJsonPath, file.errorString());
        }
        return false;
    }
    const QString strJson = QString::fromUtf8(file.readAll());
    file.close();

    Project parsed;
    if (!ProjectSerializer::fromJson(strJson, &parsed, pErrorMessage)) {
        return false;
    }
    parsed.strFilePath = QFileInfo(strJsonPath).absoluteFilePath();

    // 兼容旧项目：assets 目录缺失时自动补建
    QDir projectDir(projectDirectory());
    if (!projectDir.exists()) {
        projectDir.mkpath(QStringLiteral("."));
    }
    if (!projectDir.exists(QStringLiteral("assets"))) {
        projectDir.mkpath(QStringLiteral("assets"));
    }

    m_project = parsed;
    m_bDirty = false;
    addRecentProject(m_project.strFilePath);
    startAutoSaveTimer();
    emit projectOpened();
    return true;
}

bool ProjectManager::recoverFromSnapshot(QString* pErrorMessage)
{
    const QString strDir = projectDirectory();
    if (!hasRecoverableSnapshot(strDir)) {
        return false;
    }

    const QString strJsonPath = projectJsonPath(strDir);
    const QString strTmpPath = strJsonPath + QStringLiteral(".tmp");
    QFile tmpFile(strTmpPath);
    if (!tmpFile.open(QIODevice::ReadOnly)) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("无法读取恢复文件：%1（%2）").arg(strTmpPath, tmpFile.errorString());
        }
        return false;
    }
    const QByteArray snapshot = tmpFile.readAll();
    tmpFile.close();

    QSaveFile target(strJsonPath);
    if (!target.open(QIODevice::WriteOnly)) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("无法写入项目文件：%1（%2）").arg(strJsonPath, target.errorString());
        }
        return false;
    }
    target.write(snapshot);
    if (!target.commit()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("恢复写入失败：%1").arg(target.errorString());
        }
        return false;
    }
    QFile::remove(strTmpPath);
    return true;
}

bool ProjectManager::save(QString* pErrorMessage)
{
    if (!hasProject()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("当前没有打开的项目");
        }
        return false;
    }

    QSaveFile file(m_project.strFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("无法写入项目文件：%1（%2）").arg(m_project.strFilePath, file.errorString());
        }
        return false;
    }
    file.write(ProjectSerializer::toJson(m_project).toUtf8());
    if (!file.commit()) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("保存项目失败：%1").arg(file.errorString());
        }
        return false;
    }
    m_bDirty = false;
    return true;
}

void ProjectManager::setDirty()
{
    m_bDirty = true;
}

void ProjectManager::setAutoSaveEnabled(bool bEnabled)
{
    m_bAutoSaveEnabled = bEnabled;
    if (bEnabled) {
        startAutoSaveTimer();
    } else {
        stopAutoSaveTimer();
    }
}

bool ProjectManager::autoSaveEnabled() const
{
    return m_bAutoSaveEnabled;
}

void ProjectManager::setAutoSaveIntervalMs(int nIntervalMs)
{
    m_nAutoSaveIntervalMs = qMax(1000, nIntervalMs);
    if (m_autoSaveTimer.isActive()) {
        m_autoSaveTimer.start(m_nAutoSaveIntervalMs);
    }
}

int ProjectManager::autoSaveIntervalMs() const
{
    return m_nAutoSaveIntervalMs;
}

void ProjectManager::startAutoSaveTimer()
{
    if (m_bAutoSaveEnabled) {
        m_autoSaveTimer.start(m_nAutoSaveIntervalMs);
    }
}

void ProjectManager::stopAutoSaveTimer()
{
    m_autoSaveTimer.stop();
}

void ProjectManager::onAutoSaveTimeout()
{
    if (!hasProject() || !m_bDirty) {
        return;
    }
    QString strErrorMessage;
    const bool bOk = save(&strErrorMessage);
    emit autoSavePerformed(bOk, bOk ? QStringLiteral("已自动保存") : strErrorMessage);
}

bool ProjectManager::hasRecoverableSnapshot(const QString& strProjectDir)
{
    const QString strJsonPath = projectJsonPath(strProjectDir);
    const QString strTmpPath = strJsonPath + QStringLiteral(".tmp");
    const QFileInfo tmpInfo(strTmpPath);
    const QFileInfo jsonInfo(strJsonPath);
    // 残留 .tmp 存在，且比正式文件新，说明上一次保存未完成
    return tmpInfo.exists() && jsonInfo.exists() && tmpInfo.lastModified() > jsonInfo.lastModified();
}

QString ProjectManager::projectJsonPath(const QString& strProjectDir)
{
    return QDir(strProjectDir).filePath(QStringLiteral("project.json"));
}

} // namespace bwm
