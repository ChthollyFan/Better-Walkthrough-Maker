/**
 * @file ExportResultHelper.cpp
 * @author zhangweimu
 * @brief 导出完成提示实现。
 */
#include "export/ExportResultHelper.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>

namespace bwm {

void showExportResult(QWidget* pParent, int nCount, const QString& strDirPath,
                      const QString& strUnit)
{
    // 无父窗口时不显示提示：调用方自行处理 UI（自动化测试即传 nullptr 以避免阻塞）
    if(!pParent) {
        return;
    }

    QMessageBox box(pParent);
    box.setWindowTitle(QStringLiteral("导出完成"));
    box.setIcon(QMessageBox::Information);
    box.setText(QStringLiteral("已成功导出 %1 %2 到：\n%3")
                    .arg(nCount).arg(strUnit).arg(strDirPath));
    QPushButton* pOpenButton = box.addButton(QStringLiteral("打开目录"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Close);
    box.exec();
    if(box.clickedButton() == pOpenButton) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(strDirPath));
    }
}

} // namespace bwm
