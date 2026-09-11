/**
 * @file AssetStore.cpp
 * @author zhangweimu
 * @brief 素材存储工具实现（图片导入与路径解析）。
 */
#include "project/AssetStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

namespace bwm {

namespace {

// 项目内的素材目录名（与 ProjectManager 建项目时保持一致）
const QString kAssetsDirName = QStringLiteral("assets");

} // namespace

QString AssetStore::importImage(const QString& strSourcePath, const QString& strProjectDir,
                                QString* pErrorMessage)
{
    if (strSourcePath.isEmpty() || !QFileInfo::exists(strSourcePath)) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("图片文件不存在：%1").arg(strSourcePath);
        }
        return QString();
    }
    // 未打开项目时不复制，直接引用源文件
    if (strProjectDir.isEmpty()) {
        return strSourcePath;
    }

    const QDir projectDir(strProjectDir);
    const QString strAssetsDirPath = projectDir.filePath(kAssetsDirName);
    // 源文件已在项目 assets/ 内：无需复制，直接用原路径（避免自复制失败）
    if (QFileInfo(strSourcePath).absolutePath().compare(QDir::cleanPath(strAssetsDirPath),
                                                        Qt::CaseInsensitive) == 0) {
        return QFileInfo(strSourcePath).absoluteFilePath();
    }

    QDir assetsDir(strAssetsDirPath);
    if (!assetsDir.exists() && !assetsDir.mkpath(QStringLiteral("."))) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("无法创建素材目录：%1").arg(strAssetsDirPath);
        }
        return QString();
    }

    const QFileInfo sourceInfo(strSourcePath);
    const QString strSuffix = sourceInfo.suffix();
    QString strTargetName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!strSuffix.isEmpty()) {
        strTargetName += QLatin1Char('.') + strSuffix;
    }
    const QString strTargetPath = assetsDir.filePath(strTargetName);
    if (!QFile::copy(strSourcePath, strTargetPath)) {
        if (pErrorMessage) {
            *pErrorMessage = QStringLiteral("复制图片到项目素材目录失败：%1").arg(strTargetPath);
        }
        return QString();
    }
    return QDir::cleanPath(strTargetPath);
}

QString AssetStore::toProjectRelative(const QString& strAbsolutePath, const QString& strProjectDir)
{
    if (strAbsolutePath.isEmpty() || strProjectDir.isEmpty()) {
        return strAbsolutePath;
    }
    const QDir projectDir(QDir::cleanPath(strProjectDir));
    const QString strCleanPath = QDir::cleanPath(strAbsolutePath);
    const QString strRelative = projectDir.relativeFilePath(strCleanPath);
    // 不在项目目录内（relativeFilePath 会返回带 ../ 的路径）时退回绝对路径
    if (strRelative.isEmpty() || strRelative.startsWith(QStringLiteral(".."))) {
        return strCleanPath;
    }
    return QDir::fromNativeSeparators(strRelative);
}

QString AssetStore::resolvePath(const QString& strPath, const QString& strProjectDir)
{
    if (strPath.isEmpty()) {
        return QString();
    }
    const QFileInfo info(strPath);
    if (!info.isRelative()) {
        return strPath;   // 绝对路径（含旧数据）原样返回
    }
    if (strProjectDir.isEmpty()) {
        return strPath;   // 无项目目录可解析，交由调用方兜底
    }
    return QDir(strProjectDir).filePath(strPath);
}

} // namespace bwm
