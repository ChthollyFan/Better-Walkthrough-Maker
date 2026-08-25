/**
 * @file TemplateManager.cpp
 * @author zhangweimu
 * @brief 模板管理实现。
 */
#include "template/TemplateManager.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>

#include "core/Template.h"
#include "template/BuiltinTemplates.h"

namespace bwm {

namespace {

// 文件名净化：去掉 Windows 非法字符
QString sanitizeFileName(const QString& rName)
{
    QString strResult = rName;
    strResult.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
    return strResult.trimmed().isEmpty() ? QStringLiteral("模板") : strResult;
}

} // namespace

QString TemplateManager::templatesDirectory(const QString& rProjectDir)
{
    return QDir(rProjectDir).filePath(QStringLiteral("templates"));
}

QVector<Template> TemplateManager::userTemplates(const QString& rProjectDir)
{
    QVector<Template> vecTemplates;
    const QString strDir = templatesDirectory(rProjectDir);
    QDir dir(strDir);
    if (!dir.exists()) {
        return vecTemplates;
    }
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
    for (const QString& strFile : files) {
        Template t;
        QString strError;
        if (TemplateSerializer::readFile(dir.absoluteFilePath(strFile), &t, &strError)) {
            vecTemplates.append(t);
        }
    }
    return vecTemplates;
}

bool TemplateManager::saveTemplate(const Template& rTemplate, const QString& rProjectDir,
                                   QString* pErrorMessage)
{
    const QString strDir = templatesDirectory(rProjectDir);
    QDir dir(strDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("无法创建模板目录：%1").arg(strDir);
        }
        return false;
    }
    // 文件名唯一性：重名自动加 (2)/(3)…
    QString strFileName = sanitizeFileName(rTemplate.strName) + QStringLiteral(".json");
    int nSuffix = 2;
    while (dir.exists(strFileName)) {
        strFileName = QStringLiteral("%1 (%2).json")
                          .arg(sanitizeFileName(rTemplate.strName)).arg(nSuffix);
        ++nSuffix;
    }
    return TemplateSerializer::writeFile(rTemplate, dir.filePath(strFileName), pErrorMessage);
}

bool TemplateManager::importTemplate(const QString& rJsonPath, const QString& rProjectDir,
                                     QString* pErrorMessage)
{
    Template t;
    if (!TemplateSerializer::readFile(rJsonPath, &t, pErrorMessage)) {
        return false;
    }
    return saveTemplate(t, rProjectDir, pErrorMessage);
}

bool TemplateManager::exportTemplate(const QString& rTemplatePath, const QString& rDestPath,
                                     QString* pErrorMessage)
{
    Template t;
    if (!TemplateSerializer::readFile(rTemplatePath, &t, pErrorMessage)) {
        return false;
    }
    return TemplateSerializer::writeFile(t, rDestPath, pErrorMessage);
}

QVector<Template> TemplateManager::allTemplates(const QString& rProjectDir)
{
    QVector<Template> vecTemplates = builtinTemplates();
    const QVector<Template> userTemplatesList = userTemplates(rProjectDir);
    for (const Template& rTemplate : userTemplatesList) {
        vecTemplates.append(rTemplate);
    }
    return vecTemplates;
}

} // namespace bwm
