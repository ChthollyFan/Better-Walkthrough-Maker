/**
 * @file ExportResultHelper.h
 * @author zhangweimu
 * @brief 导出完成提示的共享实现：显示导出数量并提供"打开目录"按钮。
 *
 * 供各导出 Provider（页面型 / 文章型）共用，统一导出完成后的用户体验。
 */
#ifndef BWM_EXPORT_EXPORTRESULTHELPER_H
#define BWM_EXPORT_EXPORTRESULTHELPER_H

#include <QString>

class QWidget;

namespace bwm {

/**
 * @brief 显示导出完成提示，支持一键打开导出目录。
 *
 * @param pParent     父窗口；为 nullptr 时不显示提示（便于自动化测试与静默调用）
 * @param nCount      导出文件数
 * @param strDirPath  导出目录路径
 * @param strUnit     单位描述（如"张图片"、"个文件"）
 */
void showExportResult(QWidget* pParent, int nCount, const QString& strDirPath,
                      const QString& strUnit = QStringLiteral("张图片"));

} // namespace bwm

#endif // BWM_EXPORT_EXPORTRESULTHELPER_H
