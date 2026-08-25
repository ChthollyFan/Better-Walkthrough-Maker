/**
 * @file ProjectManager.h
 * @author zhangweimu
 * @brief 项目管理：新建 / 打开 / 保存 / 自动保存 / 崩溃恢复。
 */
#ifndef BWM_PROJECT_PROJECTMANAGER_H
#define BWM_PROJECT_PROJECTMANAGER_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "core/Project.h"

namespace bwm {

// 项目管理：新建 / 打开 / 保存 / 自动保存 / 崩溃恢复（规划第 5.1 节）。
class ProjectManager : public QObject {
    Q_OBJECT
public:
    explicit ProjectManager(QObject* pParent = nullptr);

    bool hasProject() const;
    Project* project();
    const Project* project() const;
    // 项目文件夹（project.json 所在目录）
    QString projectDirectory() const;
    bool isDirty() const;

    // 新建项目：在 strParentDirectory 下创建 <游戏名>.bwm 文件夹、assets/ 与 project.json。
    bool createProject(const QString& strGameName, const QSize& size,
                       const QString& strParentDirectory, QString* pErrorMessage);
    // 打开项目：读取 project.json，注册最近项目，启动自动保存。
    bool openProject(const QString& strJsonPath, QString* pErrorMessage);
    // 崩溃恢复：用残留的 project.json.tmp 覆盖 project.json 后打开。
    bool recoverFromSnapshot(QString* pErrorMessage);

    // 立即保存（手动保存与自动保存共用），写临时文件后原子替换。
    bool save(QString* pErrorMessage);
    // 数据变更标记；由各编辑器在修改后调用，驱动自动保存与窗口标题的脏标记。
    void setDirty();

    // 自动保存
    void setAutoSaveEnabled(bool bEnabled);
    bool autoSaveEnabled() const;
    void setAutoSaveIntervalMs(int nIntervalMs);
    int autoSaveIntervalMs() const;

    // 崩溃恢复检测：项目目录中存在比 project.json 更新的 .tmp 残留。
    static bool hasRecoverableSnapshot(const QString& strProjectDir);

signals:
    void projectOpened();
    void projectClosed();
    void autoSavePerformed(bool bOk, const QString& strMessage);

private:
    void startAutoSaveTimer();
    void stopAutoSaveTimer();
    void onAutoSaveTimeout();
    static QString projectJsonPath(const QString& strProjectDir);

    Project m_project;                           // 当前项目数据
    bool m_bDirty = false;                       // 数据是否已修改
    bool m_bAutoSaveEnabled = true;              // 自动保存开关
    int m_nAutoSaveIntervalMs = 5 * 60 * 1000;   // 保存间隔（默认 5 分钟，与全局设置一致）
    QTimer m_autoSaveTimer;                      // 自动保存定时器
};

} // namespace bwm

#endif // BWM_PROJECT_PROJECTMANAGER_H
