/**
 * @file ArticleImporter.h
 * @author zhangweimu
 * @brief Markdown 文件导入器：读取 .md 文件，复制本地图片到项目 assets/，
 *        改写图片路径为相对引用，返回 Article 数据。
 */
#ifndef BWM_EXPORT_ARTICLEIMPORTER_H
#define BWM_EXPORT_ARTICLEIMPORTER_H

#include <QString>

#include "core/Article.h"

namespace bwm {

/**
 * @brief Markdown 文件导入器（静态方法）。
 */
class ArticleImporter
{
public:
    /**
     * @brief 从 .md 文件导入文章。
     *
     * 读取 .md 内容，对其中的本地图片引用 ![](xxx.png)：
     * - 若图片与 .md 同目录，复制到项目 assets/ 并改写路径为 assets/xxx.png
     * - 外部 .md 中的 ![[W:P]] 原样保留
     *
     * @param strFilePath   .md 文件路径
     * @param strProjectDir  项目目录（assets/ 的父目录）
     * @param pError        错误信息输出（失败时填充）
     * @return              导入的 Article（strTitle 取文件名，strMarkdown 为改写后的内容）
     */
    static Article importFromFile(const QString& strFilePath,
                                   const QString& strProjectDir,
                                   QString* pError = nullptr);
};

} // namespace bwm

#endif // BWM_EXPORT_ARTICLEIMPORTER_H
