/**
 * @file ArticleImporter.cpp
 * @author zhangweimu
 * @brief Markdown 文件导入器实现。
 */
#include "export/ArticleImporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QRegularExpression>

namespace bwm {

// 普通图片语法正则：匹配 ![alt](url)
static const QRegularExpression kImageRegex(QStringLiteral(R"(!\[([^\]]*)\]\(([^)]+)\))"));

Article ArticleImporter::importFromFile(const QString& strFilePath,
                                         const QString& strProjectDir,
                                         QString* pError)
{
    Article article;

    QFile file(strFilePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if(pError) {
            *pError = QStringLiteral("无法打开文件：%1").arg(strFilePath);
        }
        return article;
    }
    QString strMarkdown = QString::fromUtf8(file.readAll());
    file.close();

    // 标题取文件名（不含扩展名）
    article.strTitle = QFileInfo(strFilePath).completeBaseName();

    // 解析并复制图片
    const QDir srcDir = QFileInfo(strFilePath).absoluteDir();
    QDir assetsDir(strProjectDir);
    if(!assetsDir.exists(QStringLiteral("assets"))) {
        assetsDir.mkpath(QStringLiteral("assets"));
    }
    assetsDir.cd(QStringLiteral("assets"));

    QString strResolved = strMarkdown;
    QRegularExpressionMatchIterator it = kImageRegex.globalMatch(strMarkdown);
    QVector<QRegularExpressionMatch> vecMatches;
    while(it.hasNext()) {
        vecMatches.append(it.next());
    }
    // 倒序替换
    for(int i = vecMatches.size() - 1; i >= 0; --i) {
        const QRegularExpressionMatch& rMatch = vecMatches.at(i);
        const QString strUrl = rMatch.captured(2);

        // 跳过网络 URL 和已是 assets/ 的路径
        if(strUrl.startsWith(QStringLiteral("http://"))
           || strUrl.startsWith(QStringLiteral("https://"))
           || strUrl.startsWith(QStringLiteral("assets/"))) {
            continue;
        }

        // 在 .md 同目录查找图片
        const QString strSrcPath = srcDir.filePath(strUrl);
        if(!QFileInfo(strSrcPath).exists()) {
            continue;
        }

        // 复制到 assets/，用 UUID 避免重名
        const QFileInfo srcInfo(strSrcPath);
        const QString strDestName = QUuid::createUuid().toString(QUuid::WithoutBraces)
            + QLatin1Char('.') + srcInfo.suffix();
        const QString strDestPath = assetsDir.filePath(strDestName);
        if(!QFile::copy(strSrcPath, strDestPath)) {
            continue;
        }

        // 改写路径
        const QString strNewUrl = QStringLiteral("assets/") + strDestName;
        QString strNewMarkdown = rMatch.captured(0);
        strNewMarkdown.replace(strUrl, strNewUrl);
        strResolved.replace(rMatch.capturedStart(), rMatch.capturedLength(), strNewMarkdown);
    }

    article.strMarkdown = strResolved;
    article.strId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return article;
}

} // namespace bwm
