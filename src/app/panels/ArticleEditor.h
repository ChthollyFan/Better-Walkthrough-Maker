/**
 * @file ArticleEditor.h
 * @author zhangweimu
 * @brief 文章攻略编辑器：左侧 Markdown 源码编辑 + 右侧实时预览（分栏）。
 *
 * 功能：
 * - 左侧 QTextEdit 编辑 Markdown 源码
 * - 右侧 MarkdownPreview 实时渲染预览（防抖 300ms）
 * - 工具栏：加粗/斜体/标题/列表/链接/图片/页面引用 插入按钮
 * - 页面引用 ![[W:P]] 在预览中渲染为对应图文攻略页面的图片
 * - 通过 loadArticle() 加载指定文章，编辑后通知 MainWindow 同步模型并标记 dirty
 */
#ifndef BWM_APP_PANELS_ARTICLEEDITOR_H
#define BWM_APP_PANELS_ARTICLEEDITOR_H

#include <QWidget>

#include "core/Article.h"

class QTextEdit;
class QTimer;

namespace bwm {

class Project;
class ProjectManager;
class MarkdownPreview;

/**
 * @brief 文章攻略编辑器（分栏：左编辑右预览）。
 *
 * 选中文章节点时由 MainWindow 切换到此面板。
 * 编辑内容后发出 articleModified 信号，MainWindow 据此同步模型并标记 dirty。
 */
class ArticleEditor : public QWidget
{
    Q_OBJECT
public:
    /**
     * @param pParent           父 widget
     * @param pProjectManager   项目管理器（获取项目目录用于图片路径解析）
     */
    ArticleEditor(QWidget* pParent, ProjectManager* pProjectManager);

    /**
     * @brief 设置项目上下文（Project 指针与主题背景色），供预览渲染页面引用。
     *        应在项目打开或主题变更后调用。
     */
    void setProjectContext(const Project* pProject, const QColor& rBackgroundColor);

    /**
     * @brief 加载指定文章到编辑器。
     * @param rArticle  文章数据（编辑器持有副本，编辑时写回模型）
     */
    void loadArticle(const Article& rArticle);

    /**
     * @brief 清空编辑器（无选中文章时）。
     */
    void clear();

signals:
    /**
     * @brief 文章内容被修改（用户编辑了 Markdown 源码）。
     * MainWindow 接收后同步回模型并标记 dirty。
     * @param rMarkdown  当前编辑器中的 Markdown 源码
     */
    void articleModified(const QString& rMarkdown);

private slots:
    // 编辑器内容变化（防抖触发预览刷新）
    void onSourceChanged();
    // 防抖定时器到期：刷新预览并发出 articleModified
    void onDebounceTimeout();

    // 工具栏插入按钮
    void onInsertBold();
    void onInsertItalic();
    void onInsertHeading();
    void onInsertList();
    void onInsertLink();
    void onInsertImage();
    void onInsertPageRef();

private:
    void createToolBar();
    void refreshPreview();
    // 在编辑器光标处插入 Markdown 标记，支持选中文本包裹
    void insertMarkdownWrap(const QString& strBefore, const QString& strAfter);
    // 在行首插入前缀（如 # / - ）
    void insertLinePrefix(const QString& strPrefix);

    ProjectManager* m_pProjectManager;  ///< 项目管理器
    QTextEdit* m_pSourceEdit;           ///< 左侧 Markdown 源码编辑器
    MarkdownPreview* m_pPreview;        ///< 右侧预览控件
    QTimer* m_pDebounceTimer;           ///< 防抖定时器（300ms）
    bool m_bLoading;                    ///< 加载文章时禁止触发 articleModified
};

} // namespace bwm

#endif // BWM_APP_PANELS_ARTICLEEDITOR_H
