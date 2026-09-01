/**
 * @file SettingsDialog.cpp
 * @author zhangweimu
 * @brief 设置对话框实现。
 */
#include "app/dialogs/SettingsDialog.h"

#include "project/ProjectManager.h"
#include "settings/Settings.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

namespace bwm {

class SettingsDialog::Ui
{
public:
    QComboBox* pSizeCombo = nullptr;       ///< 默认画布尺寸
    QSpinBox* pAutoSaveSpin = nullptr;     ///< 自动保存间隔（分钟）
    QLineEdit* pAuthorEdit = nullptr;      ///< 作者署名
};

SettingsDialog::SettingsDialog(QWidget* pParent, ProjectManager* pProjectManager)
    : QDialog(pParent)
    , m_pUi(new Ui)
    , m_pProjectManager(pProjectManager)
{
    setWindowTitle(QStringLiteral("设置"));
    auto* pFormLayout = new QFormLayout(this);

    // 默认画布尺寸
    m_pUi->pSizeCombo = new QComboBox(this);
    const QList<QPair<QString, QSize>> presets = {
        {QStringLiteral("竖图 1080×1440"), QSize(1080, 1440)},
        {QStringLiteral("横图 1920×1080"), QSize(1920, 1080)},
        {QStringLiteral("方形 1080×1080"), QSize(1080, 1080)},
        {QStringLiteral("长图 1080×2400"), QSize(1080, 2400)},
    };
    const QSize currentSize = Settings::defaultPageSize();
    int nCurrentIndex = 0;
    for(int nIndex = 0; nIndex < presets.size(); ++nIndex) {
        m_pUi->pSizeCombo->addItem(presets.at(nIndex).first, presets.at(nIndex).second);
        if(presets.at(nIndex).second == currentSize) {
            nCurrentIndex = nIndex;
        }
    }
    m_pUi->pSizeCombo->setCurrentIndex(nCurrentIndex);
    pFormLayout->addRow(QStringLiteral("默认画布尺寸："), m_pUi->pSizeCombo);

    // 自动保存间隔（分钟）
    m_pUi->pAutoSaveSpin = new QSpinBox(this);
    m_pUi->pAutoSaveSpin->setRange(1, 60);
    m_pUi->pAutoSaveSpin->setValue(qMax(1, Settings::autoSaveIntervalMs() / 60000));
    pFormLayout->addRow(QStringLiteral("自动保存间隔（分钟）："), m_pUi->pAutoSaveSpin);

    // 作者署名
    m_pUi->pAuthorEdit = new QLineEdit(this);
    m_pUi->pAuthorEdit->setPlaceholderText(QStringLiteral("如小黑盒 ID"));
    m_pUi->pAuthorEdit->setText(Settings::authorName());
    pFormLayout->addRow(QStringLiteral("作者署名："), m_pUi->pAuthorEdit);

    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    pButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    pButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(pButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(pButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    pFormLayout->addRow(pButtons);
}

void SettingsDialog::applySettings() const
{
    Settings::setDefaultPageSize(m_pUi->pSizeCombo->currentData().toSize());
    Settings::setAutoSaveIntervalMs(m_pUi->pAutoSaveSpin->value() * 60000);
    Settings::setAuthorName(m_pUi->pAuthorEdit->text().trimmed());
    if(m_pProjectManager && m_pProjectManager->hasProject()) {
        m_pProjectManager->setAutoSaveIntervalMs(Settings::autoSaveIntervalMs());
    }
}

} // namespace bwm
