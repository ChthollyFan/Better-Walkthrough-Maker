#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "core/Project.h"

namespace bwm {

// 项目管理：新建 / 打开 / 保存 / 自动保存 / 崩溃恢复（规划第 5.1 节）。
class ProjectManager : public QObject {
    Q_OBJECT
public:
    explicit ProjectManager(QObject *parent = nullptr);

    bool hasProject() const;
    Project *project();
    const Project *project() const;
    // 项目文件夹（project.json 所在目录）
    QString projectDirectory() const;
    bool isDirty() const { return m_dirty; }

    // 新建项目：在 parentDirectory 下创建 <游戏名>.bwm 文件夹、assets/ 与 project.json。
    bool createProject(const QString &gameName, const QSize &defaultPageSize,
                       const QString &parentDirectory, QString *errorMessage);
    // 打开项目：读取 project.json，注册最近项目，启动自动保存。
    bool openProject(const QString &jsonPath, QString *errorMessage);
    // 崩溃恢复：用残留的 project.json.tmp 覆盖 project.json 后打开。
    bool recoverFromSnapshot(QString *errorMessage);

    // 立即保存（手动保存与自动保存共用），写临时文件后原子替换。
    bool save(QString *errorMessage);
    // 数据变更标记；由各编辑器在修改后调用，驱动自动保存与窗口标题的脏标记。
    void setDirty();

    // 自动保存
    void setAutoSaveEnabled(bool enabled);
    bool autoSaveEnabled() const { return m_autoSaveEnabled; }
    void setAutoSaveIntervalMs(int intervalMs);
    int autoSaveIntervalMs() const { return m_autoSaveIntervalMs; }

    // 崩溃恢复检测：项目目录中存在比 project.json 更新的 .tmp 残留。
    static bool hasRecoverableSnapshot(const QString &projectDir);

signals:
    void projectOpened();
    void projectClosed();
    void autoSavePerformed(bool ok, const QString &message);

private:
    void startAutoSaveTimer();
    void stopAutoSaveTimer();
    void onAutoSaveTimeout();
    static QString projectJsonPath(const QString &projectDir);

    Project m_project;
    bool m_dirty = false;
    bool m_autoSaveEnabled = true;
    int m_autoSaveIntervalMs = 5 * 60 * 1000; // 默认 5 分钟，与全局设置一致
    QTimer m_autoSaveTimer;
};

} // namespace bwm
