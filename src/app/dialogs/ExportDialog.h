/**
 * @file ExportDialog.h
 * @author zhangweimu
 * @brief 导出对话框：选择导出范围、格式、倍率、署名、目录后执行导出。
 *
 * 从原 MainWindow::onExportPng 拆分。与原实现不同：
 * 导出格式不再硬编码，而是从 PluginHost 的 exportProviders() 列表动态填充。
 * 用户确认后调用对应 IExportProvider::exportPages 执行导出。
 *
 * 导出范围：
 * - 当前页：仅导出选中页面
 * - 当前攻略全部页：导出选中攻略的全部页面
 */
#ifndef BWM_APP_DIALOGS_EXPORTDIALOG_H
#define BWM_APP_DIALOGS_EXPORTDIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>

#include "core/Project.h"
#include "plugin/PluginContext.h"

namespace bwm {

class ProjectManager;
class PluginHost;
class IExportProvider;

/**
 * @brief 导出对话框。
 *
 * 显示导出选项，用户确认后调用选定 Provider 执行导出。
 * 长图格式仅在"全部页"范围时有效。
 */
class ExportDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @param pParent           父窗口
     * @param pProjectManager   项目管理器
     * @param pHost             插件宿主（获取导出格式列表）
     * @param rContext          插件上下文（主题背景色等）
     */
    ExportDialog(QWidget* pParent, ProjectManager* pProjectManager, PluginHost* pHost,
                 const PluginContext& rContext);

    /**
     * @brief 设置当前选中页面键（"W:P" 格式），用于"当前页"范围导出。
     */
    void setCurrentPageKey(const QString& rKey);

private slots:
    void onAccept();

private:
    class Ui;
    Ui* m_pUi;
    ProjectManager* m_pProjectManager;   ///< 项目管理器
    PluginHost* m_pHost;                 ///< 插件宿主
    PluginContext m_context;             ///< 插件上下文
    QString m_strCurrentPageKey;         ///< 当前选中页面键
};

} // namespace bwm

#endif // BWM_APP_DIALOGS_EXPORTDIALOG_H
