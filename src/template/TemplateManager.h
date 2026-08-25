/**
 * @file TemplateManager.h
 * @author zhangweimu
 * @brief 模板管理：项目内用户模板的保存/导入/导出与内置模板合并。
 */
#ifndef BWM_TEMPLATE_TEMPLATEMANAGER_H
#define BWM_TEMPLATE_TEMPLATEMANAGER_H

#include <QString>
#include <QVector>

#include "core/Template.h"

namespace bwm {

// 模板管理：用户模板存于项目目录 templates/ 下（.json 文件）。
class TemplateManager {
public:
    // 项目模板目录（不存在时返回空字符串，需要时由调用方创建）
    static QString templatesDirectory(const QString& rProjectDir);
    // 扫描并读取项目内全部用户模板
    static QVector<Template> userTemplates(const QString& rProjectDir);
    // 保存模板到项目 templates/（文件名 = 模板名.json，重名自动加后缀）
    static bool saveTemplate(const Template& rTemplate, const QString& rProjectDir,
                             QString* pErrorMessage);
    // 导入模板文件到项目 templates/
    static bool importTemplate(const QString& rJsonPath, const QString& rProjectDir,
                               QString* pErrorMessage);
    // 导出模板到指定目标路径
    static bool exportTemplate(const QString& rTemplatePath, const QString& rDestPath,
                               QString* pErrorMessage);
    // 全部模板：内置 + 项目用户模板
    static QVector<Template> allTemplates(const QString& rProjectDir);
};

} // namespace bwm

#endif // BWM_TEMPLATE_TEMPLATEMANAGER_H
