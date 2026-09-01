/**
 * @file SettingsDialog.h
 * @author zhangweimu
 * @brief 设置对话框：默认画布尺寸、自动保存间隔、作者署名。
 *
 * 从原 MainWindow::onShowSettings 拆分。
 * exec() 后若结果为 Accepted，调用 applySettings() 持久化设置。
 */
#ifndef BWM_APP_DIALOGS_SETTINGSDIALOG_H
#define BWM_APP_DIALOGS_SETTINGSDIALOG_H

#include <QDialog>

namespace bwm {

class ProjectManager;

/**
 * @brief 设置对话框。
 * 确认后调用 applySettings 把结果写入 Settings，并更新 ProjectManager 的自动保存间隔。
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @param pParent           父窗口
     * @param pProjectManager   项目管理器（更新自动保存间隔，可为 nullptr）
     */
    explicit SettingsDialog(QWidget* pParent, ProjectManager* pProjectManager = nullptr);

    /**
     * @brief 将对话框结果写入 Settings 并更新 ProjectManager。
     * 在 exec() 返回 Accepted 后调用。
     */
    void applySettings() const;

private:
    class Ui;
    Ui* m_pUi;
    ProjectManager* m_pProjectManager;
};

} // namespace bwm

#endif // BWM_APP_DIALOGS_SETTINGSDIALOG_H
