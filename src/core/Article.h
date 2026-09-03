/**
 * @file Article.h
 * @author zhangweimu
 * @brief 文章攻略数据模型：基于 Markdown 的文本攻略，与 Walkthrough（图文攻略）平级。
 *
 * Article 与 Walkthrough 平级，同属 Project 下的攻略形态。区别在于：
 * - Walkthrough：画布 + 组件，导出为 PNG（图文攻略）
 * - Article：Markdown 富文本，支持嵌入图片与引用图文攻略页面，导出为 .md / PNG / PDF
 *
 * 页面引用语法（本项目自定义扩展，标准 Markdown 不支持）：
 *   ![[W:P]]  表示引用第 W 个图文攻略的第 P 页
 * 预览与导出时解析此语法，用 ExportRenderer::renderPage() 渲染对应页面并嵌入。
 */
#ifndef BWM_CORE_ARTICLE_H
#define BWM_CORE_ARTICLE_H

#include <QString>

namespace bwm {

/**
 * @brief 文章攻略：基于 Markdown 的文本攻略。
 *
 * 正文为 Markdown 源码（字符串），内嵌于 project.json，随项目自包含。
 * 可包含 ![[W:P]] 页面引用与 ![](assets/...) 图片引用。
 */
struct Article {
    QString strId;           ///< 唯一 id（QUuid 字符串）
    QString strTitle;        ///< 文章标题
    QString strMarkdown;     ///< Markdown 正文（内嵌，含页面引用与图片引用）
};

/**
 * @brief 页面引用语法的正则匹配模式字符串。
 *
 * 形如 ![[0:1]] 表示引用第 0 个图文攻略的第 1 页。
 * 解析方使用 QRegularExpression(thisPattern) 提取 W 与 P。
 */
const QString kPageRefPattern = QString::fromLatin1(R"(!\[\[(\d+):(\d+)\]\])");

/**
 * @brief 生成页面引用标记文本（如 ![[0:1]]）。
 * @param nWalkthroughIndex  图文攻略在 vecWalkthroughs 中的索引
 * @param nPageIndex         页面在该攻略 vecPages 中的索引
 * @return                   形如 ![[0:1]] 的标记字符串
 */
QString makePageRef(int nWalkthroughIndex, int nPageIndex);

/**
 * @brief 判断字符串中是否包含页面引用标记。
 */
bool containsPageRef(const QString& rText);

} // namespace bwm

#endif // BWM_CORE_ARTICLE_H
