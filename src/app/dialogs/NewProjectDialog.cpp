/**
 * @file NewProjectDialog.cpp
 * @author zhangweimu
 * @brief 新建项目对话框实现。
 */
#include "app/dialogs/NewProjectDialog.h"

#include "settings/Settings.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

namespace bwm {

/**
 * @brief UI 组件集合：把对话框内各控件集中管理，避免头文件暴露 Qt 细节。
 */
class NewProjectDialog::Ui
{
public:
    QLineEdit* pNameEdit = nullptr;       ///< 游戏名输入框
    QComboBox* pSizeCombo = nullptr;      ///< 画布尺寸下拉框
};

NewProjectDialog::NewProjectDialog(QWidget* pParent)
    : QDialog(pParent)
    , m_pUi(new Ui)
{
    setWindowTitle(QStringLiteral("新建项目"));

    m_pUi->pNameEdit = new QLineEdit(this);
    m_pUi->pNameEdit->setPlaceholderText(QStringLiteral("游戏名，如：艾尔登法环"));

    m_pUi->pSizeCombo = new QComboBox(this);
    const QSize defaultSize = Settings::defaultPageSize();
    const QList<QPair<QString, QSize>> presets = {
        {QStringLiteral("竖图 1080×1440（默认）"), QSize(1080, 1440)},
        {QStringLiteral("横图 1920×1080"), QSize(1920, 1080)},
        {QStringLiteral("方形 1080×1080"), QSize(1080, 1080)},
        {QStringLiteral("长图 1080×2400"), QSize(1080, 2400)},
    };
    int nSelectedIndex = 0;
    for(int nIndex = 0; nIndex < presets.size(); ++nIndex) {
        m_pUi->pSizeCombo->addItem(presets.at(nIndex).first, presets.at(nIndex).second);
        if(presets.at(nIndex).second == defaultSize) {
            nSelectedIndex = nIndex;
        }
    }
    m_pUi->pSizeCombo->setCurrentIndex(nSelectedIndex);

    auto* pForm = new QFormLayout(this);
    pForm->addRow(QStringLiteral("游戏名："), m_pUi->pNameEdit);
    pForm->addRow(QStringLiteral("默认画布尺寸："), m_pUi->pSizeCombo);

    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    pButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("创建"));
    pButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(pButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    pForm->addRow(pButtons);
}

QString NewProjectDialog::gameName() const
{
    return m_pUi->pNameEdit->text().trimmed();
}

QSize NewProjectDialog::pageSize() const
{
    return m_pUi->pSizeCombo->currentData().toSize();
}

} // namespace bwm
