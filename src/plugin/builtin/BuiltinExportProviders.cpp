/**
 * @file BuiltinExportProviders.cpp
 * @author zhangweimu
 * @brief 内置导出格式适配器实现。
 *
 * 导出逻辑迁移自原 MainWindow::onExportPng 的逐页/长图分支。
 * 文件名净化逻辑（去掉 Windows 非法字符）也迁移至此。
 */
#include "plugin/builtin/BuiltinExportProviders.h"

#include "export/ExportRenderer.h"

#include <QDesktopServices>
#include <QDir>
#include <QImage>
#include <QMessageBox>
#include <QPushButton>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QUrl>

namespace bwm {

// 匿名命名空间：文件内辅助函数
namespace {

/**
 * @brief 文件名净化：将 Windows 非法字符替换为下划线。
 * @param rTitle  原始标题
 * @return        净化后的安全文件名（空标题返回"攻略"）
 */
QString sanitizeFileName(const QString& rTitle)
{
    QString strSafe = rTitle;
    strSafe.replace(QRegularExpression(QStringLiteral(R"([\\/:*?\"<>|])")), QStringLiteral("_"));
    if(strSafe.trimmed().isEmpty()) {
        strSafe = QStringLiteral("攻略");
    }
    return strSafe;
}

/**
 * @brief 显示导出完成提示，支持一键打开导出目录。
 * @param pParent     父窗口
 * @param nCount      导出文件数
 * @param strDirPath  导出目录路径
 */
void showExportResult(QWidget* pParent, int nCount, const QString& strDirPath)
{
    QMessageBox box(pParent);
    box.setWindowTitle(QStringLiteral("导出完成"));
    box.setIcon(QMessageBox::Information);
    box.setText(QStringLiteral("已成功导出 %1 张图片到：\n%2").arg(nCount).arg(strDirPath));
    QPushButton* pOpenButton = box.addButton(QStringLiteral("打开目录"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Close);
    box.exec();
    if(box.clickedButton() == pOpenButton) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(strDirPath));
    }
}

} // namespace

// =========================================================================
// PngSeparateExportProvider
// =========================================================================

QString PngSeparateExportProvider::formatId() const
{
    return QStringLiteral("builtin.png.separate");
}

QString PngSeparateExportProvider::displayName() const
{
    return QStringLiteral("逐页 PNG");
}

int PngSeparateExportProvider::exportPages(const QVector<Page>& vecPages,
                                           const QString& rWalkthroughTitle,
                                           const QString& strDirPath,
                                           qreal dScale,
                                           const QString& strAuthor,
                                           const PluginContext& rContext,
                                           QWidget* pParent) const
{
    const QString strSafeTitle = sanitizeFileName(rWalkthroughTitle);
    const QString strCleanDir = QDir::cleanPath(strDirPath);

    // 逐页导出：显示进度对话框
    QProgressDialog progress(QStringLiteral("正在导出…"), QString(), 0, vecPages.size(), pParent);
    progress.setWindowModality(Qt::WindowModal);
    int nExported = 0;
    for(int nIndex = 0; nIndex < vecPages.size(); ++nIndex) {
        progress.setValue(nIndex);
        if(progress.wasCanceled()) {
            break;
        }
        const QImage image = ExportRenderer::renderPage(vecPages.at(nIndex), dScale,
                                                        rContext.theme.backgroundColor,
                                                        strAuthor);
        const QString strFileName = QStringLiteral("%1_%2.png")
                                        .arg(strSafeTitle)
                                        .arg(nIndex + 1, 2, 10, QLatin1Char('0'));
        if(ExportRenderer::writePng(image, QDir(strCleanDir).filePath(strFileName), nullptr)) {
            ++nExported;
        }
    }
    progress.setValue(vecPages.size());

    if(nExported > 0) {
        showExportResult(pParent, nExported, strCleanDir);
    }
    return nExported;
}

// =========================================================================
// PngLongImageExportProvider
// =========================================================================

QString PngLongImageExportProvider::formatId() const
{
    return QStringLiteral("builtin.png.longimage");
}

QString PngLongImageExportProvider::displayName() const
{
    return QStringLiteral("长图（纵向拼接）");
}

int PngLongImageExportProvider::exportPages(const QVector<Page>& vecPages,
                                            const QString& rWalkthroughTitle,
                                            const QString& strDirPath,
                                            qreal dScale,
                                            const QString& strAuthor,
                                            const PluginContext& rContext,
                                            QWidget* pParent) const
{
    const QString strSafeTitle = sanitizeFileName(rWalkthroughTitle);
    const QString strCleanDir = QDir::cleanPath(strDirPath);

    // 渲染长图（纵向拼接，带分隔线）
    QString strErrorMessage;
    const QImage image = ExportRenderer::renderLongImage(
        vecPages, dScale, true, &strErrorMessage,
        rContext.theme.backgroundColor, strAuthor);
    if(image.isNull()) {
        QMessageBox::critical(pParent, QStringLiteral("导出 PNG"), strErrorMessage);
        return 0;
    }

    // 显示进度（长图渲染是同步的，进度仅为占位）
    QProgressDialog progress(QStringLiteral("正在导出长图…"), QString(), 0, 1, pParent);
    progress.setWindowModality(Qt::WindowModal);
    progress.setValue(0);

    const QString strFilePath = QDir(strCleanDir).filePath(
        strSafeTitle + QStringLiteral("_长图.png"));
    QString strWriteError;
    if(!ExportRenderer::writePng(image, strFilePath, &strWriteError)) {
        QMessageBox::critical(pParent, QStringLiteral("导出 PNG"), strWriteError);
        return 0;
    }
    progress.setValue(1);
    showExportResult(pParent, 1, strCleanDir);
    return 1;
}

} // namespace bwm
