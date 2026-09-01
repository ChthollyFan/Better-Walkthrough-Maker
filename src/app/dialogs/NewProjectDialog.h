/**
 * @file NewProjectDialog.h
 * @author zhangweimu
 * @brief 新建项目对话框：输入游戏名 + 选择默认画布尺寸。
 *
 * 从原 MainWindow::onNewProject 的对话框部分拆分。
 * 纯数据对话框，exec() 后通过 gameName()/pageSize() 获取结果。
 */
#ifndef BWM_APP_DIALOGS_NEWPROJECTDIALOG_H
#define BWM_APP_DIALOGS_NEWPROJECTDIALOG_H

#include <QDialog>
#include <QSize>
#include <QString>

namespace bwm {

/**
 * @brief 新建项目对话框。
 * 用户输入游戏名并选择默认画布尺寸，确认后通过访问器获取结果。
 */
class NewProjectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NewProjectDialog(QWidget* pParent = nullptr);

    /** @brief 用户输入的游戏名（已 trim） */
    QString gameName() const;

    /** @brief 用户选择的默认画布尺寸 */
    QSize pageSize() const;

private:
    class Ui;                    ///< UI 构建辅助（前向声明）
    Ui* m_pUi;                   ///< UI 组件指针集合
};

} // namespace bwm

#endif // BWM_APP_DIALOGS_NEWPROJECTDIALOG_H
