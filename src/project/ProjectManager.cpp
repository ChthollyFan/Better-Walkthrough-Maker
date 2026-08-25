#include "project/ProjectManager.h"

#include "core/ProjectSerializer.h"
#include "settings/Settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace bwm {

namespace {
constexpr int kMaxRecentProjects = 10;

// 将项目路径加入最近项目列表（内部辅助函数）
void addRecentProject(const QString &jsonPath)
{
    QStringList recents = Settings::recentProjects();
    recents.removeAll(jsonPath);
    recents.prepend(jsonPath);
    while (recents.size() > kMaxRecentProjects)
        recents.removeLast();
    Settings::setRecentProjects(recents);
}
}

ProjectManager::ProjectManager(QObject *parent)
    : QObject(parent)
{
    m_autoSaveTimer.setSingleShot(false);
    connect(&m_autoSaveTimer, &QTimer::timeout, this, &ProjectManager::onAutoSaveTimeout);
}

bool ProjectManager::hasProject() const
{
    return !m_project.filePath.isEmpty();
}

Project *ProjectManager::project()
{
    return &m_project;
}

const Project *ProjectManager::project() const
{
    return &m_project;
}

QString ProjectManager::projectDirectory() const
{
    return QFileInfo(m_project.filePath).absolutePath();
}

bool ProjectManager::createProject(const QString &gameName, const QSize &defaultPageSize,
                                   const QString &parentDirectory, QString *errorMessage)
{
    if (gameName.trimmed().isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("游戏名不能为空");
        return false;
    }

    const QDir parentDir(parentDirectory);
    if (!parentDir.exists()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("目录不存在：%1").arg(parentDirectory);
        return false;
    }

    const QString projectDirName = gameName.trimmed() + QStringLiteral(".bwm");
    const QString projectDirPath = parentDir.filePath(projectDirName);
    QDir projectDir(projectDirPath);
    if (projectDir.exists()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("项目已存在：%1").arg(projectDirPath);
        return false;
    }

    if (!projectDir.mkpath(projectDirPath) || !projectDir.mkpath(QStringLiteral("assets"))) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法创建项目目录：%1").arg(projectDirPath);
        return false;
    }

    m_project = Project();
    m_project.name = gameName.trimmed();
    m_project.filePath = projectJsonPath(projectDirPath);

    // 首个攻略与页面：让新建的项目立刻可用（名称与类型可后续修改）
    Walkthrough initialWalkthrough;
    initialWalkthrough.title = QStringLiteral("攻略 1");
    initialWalkthrough.type = WalkthroughType::Cover;
    Page initialPage;
    initialPage.name = QStringLiteral("页面 1");
    initialPage.size = defaultPageSize.isValid() && defaultPageSize.width() > 0 && defaultPageSize.height() > 0
        ? defaultPageSize
        : Settings::defaultPageSize();
    initialWalkthrough.pages.append(initialPage);
    m_project.walkthroughs.append(initialWalkthrough);

    m_dirty = true;
    if (!save(errorMessage))
        return false;

    startAutoSaveTimer();
    emit projectOpened();
    return true;
}

bool ProjectManager::openProject(const QString &jsonPath, QString *errorMessage)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法打开文件：%1（%2）").arg(jsonPath, file.errorString());
        return false;
    }
    const QString json = QString::fromUtf8(file.readAll());
    file.close();

    Project parsed;
    if (!ProjectSerializer::fromJson(json, &parsed, errorMessage))
        return false;
    parsed.filePath = QFileInfo(jsonPath).absoluteFilePath();

    // 兼容旧项目：assets 目录缺失时自动补建
    QDir projectDir(projectDirectory());
    if (!projectDir.exists())
        projectDir.mkpath(QStringLiteral("."));
    if (!projectDir.exists(QStringLiteral("assets")))
        projectDir.mkpath(QStringLiteral("assets"));

    m_project = parsed;
    m_dirty = false;
    addRecentProject(m_project.filePath);
    startAutoSaveTimer();
    emit projectOpened();
    return true;
}

bool ProjectManager::recoverFromSnapshot(QString *errorMessage)
{
    const QString dir = projectDirectory();
    if (!hasRecoverableSnapshot(dir))
        return false;

    const QString jsonPath = projectJsonPath(dir);
    const QString tmpPath = jsonPath + QStringLiteral(".tmp");
    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法读取恢复文件：%1（%2）").arg(tmpPath, tmpFile.errorString());
        return false;
    }
    const QByteArray snapshot = tmpFile.readAll();
    tmpFile.close();

    QSaveFile target(jsonPath);
    if (!target.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法写入项目文件：%1（%2）").arg(jsonPath, target.errorString());
        return false;
    }
    target.write(snapshot);
    if (!target.commit()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("恢复写入失败：%1").arg(target.errorString());
        return false;
    }
    QFile::remove(tmpPath);
    return true;
}

bool ProjectManager::save(QString *errorMessage)
{
    if (!hasProject()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("当前没有打开的项目");
        return false;
    }

    QSaveFile file(m_project.filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法写入项目文件：%1（%2）").arg(m_project.filePath, file.errorString());
        return false;
    }
    file.write(ProjectSerializer::toJson(m_project).toUtf8());
    if (!file.commit()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("保存项目失败：%1").arg(file.errorString());
        return false;
    }
    m_dirty = false;
    return true;
}

void ProjectManager::setDirty()
{
    m_dirty = true;
}

void ProjectManager::setAutoSaveEnabled(bool enabled)
{
    m_autoSaveEnabled = enabled;
    if (enabled)
        startAutoSaveTimer();
    else
        stopAutoSaveTimer();
}

void ProjectManager::setAutoSaveIntervalMs(int intervalMs)
{
    m_autoSaveIntervalMs = qMax(1000, intervalMs);
    if (m_autoSaveTimer.isActive())
        m_autoSaveTimer.start(m_autoSaveIntervalMs);
}

void ProjectManager::startAutoSaveTimer()
{
    if (m_autoSaveEnabled)
        m_autoSaveTimer.start(m_autoSaveIntervalMs);
}

void ProjectManager::stopAutoSaveTimer()
{
    m_autoSaveTimer.stop();
}

void ProjectManager::onAutoSaveTimeout()
{
    if (!hasProject() || !m_dirty)
        return;
    QString errorMessage;
    const bool ok = save(&errorMessage);
    emit autoSavePerformed(ok, ok ? QStringLiteral("已自动保存") : errorMessage);
}

bool ProjectManager::hasRecoverableSnapshot(const QString &projectDir)
{
    const QString jsonPath = projectJsonPath(projectDir);
    const QString tmpPath = jsonPath + QStringLiteral(".tmp");
    const QFileInfo tmpInfo(tmpPath);
    const QFileInfo jsonInfo(jsonPath);
    // 残留 .tmp 存在，且比正式文件新，说明上一次保存未完成
    return tmpInfo.exists() && jsonInfo.exists() && tmpInfo.lastModified() > jsonInfo.lastModified();
}

QString ProjectManager::projectJsonPath(const QString &projectDir)
{
    return QDir(projectDir).filePath(QStringLiteral("project.json"));
}

} // namespace bwm
