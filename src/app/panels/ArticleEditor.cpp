/**
 * @file ArticleEditor.cpp
 * @author zhangweimu
 * @brief 文章攻略编辑器实现。
 */
#include "app/panels/ArticleEditor.h"

#include "app/panels/MarkdownPreview.h"
#include "project/ProjectManager.h"

#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QSplitter>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QUuid>
#include <QVBoxLayout>

namespace bwm {

ArticleEditor::ArticleEditor(QWidget* pParent, ProjectManager* pProjectManager)
    : QWidget(pParent)
    , m_pProjectManager(pProjectManager)
    , m_pSourceEdit(new QTextEdit(this))
    , m_pPreview(new MarkdownPreview(this))
    , m_pDebounceTimer(new QTimer(this))
    , m_bLoading(false)
{
    // 防抖定时器：编辑后 300ms 触发预览刷新与模型同步
    m_pDebounceTimer->setSingleShot(true);
    m_pDebounceTimer->setInterval(300);

    // 设置项目目录给预览控件（图片路径解析用）
    if(m_pProjectManager && m_pProjectManager->hasProject()) {
        m_pPreview->setProjectDirectory(m_pProjectManager->projectDirectory());
    }

    // 源码编辑器：等宽字体，适合编辑 Markdown
    QFont monoFont(QStringLiteral("Consolas"), 11);
    monoFont.setStyleHint(QFont::Monospace);
    m_pSourceEdit->setFont(monoFont);
    m_pSourceEdit->setPlaceholderText(QStringLiteral("在此输入 Markdown 文章内容..."));

    // 分栏布局：工具栏 + [左编辑 | 右预览]
    auto* pMainLayout = new QVBoxLayout(this);
    pMainLayout->setContentsMargins(0, 0, 0, 0);

    createToolBar();
    // 注意：工具栏由 createToolBar 创建并添加到布局

    auto* pSplitter = new QSplitter(Qt::Horizontal, this);
    pSplitter->addWidget(m_pSourceEdit);
    pSplitter->addWidget(m_pPreview);
    pSplitter->setStretchFactor(0, 1);
    pSplitter->setStretchFactor(1, 1);
    pSplitter->setSizes({500, 500});
    pMainLayout->addWidget(pSplitter);

    // 编辑器内容变化 → 防抖
    connect(m_pSourceEdit, &QTextEdit::textChanged, this, &ArticleEditor::onSourceChanged);
    connect(m_pDebounceTimer, &QTimer::timeout, this, &ArticleEditor::onDebounceTimeout);
}

void ArticleEditor::createToolBar()
{
    auto* pToolBar = new QToolBar(this);
    pToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto* pBoldAction = pToolBar->addAction(QStringLiteral("加粗"));
    connect(pBoldAction, &QAction::triggered, this, &ArticleEditor::onInsertBold);

    auto* pItalicAction = pToolBar->addAction(QStringLiteral("斜体"));
    connect(pItalicAction, &QAction::triggered, this, &ArticleEditor::onInsertItalic);

    pToolBar->addSeparator();

    auto* pHeadingAction = pToolBar->addAction(QStringLiteral("标题"));
    connect(pHeadingAction, &QAction::triggered, this, &ArticleEditor::onInsertHeading);

    auto* pListAction = pToolBar->addAction(QStringLiteral("列表"));
    connect(pListAction, &QAction::triggered, this, &ArticleEditor::onInsertList);

    pToolBar->addSeparator();

    auto* pLinkAction = pToolBar->addAction(QStringLiteral("链接"));
    connect(pLinkAction, &QAction::triggered, this, &ArticleEditor::onInsertLink);

    auto* pImageAction = pToolBar->addAction(QStringLiteral("图片"));
    connect(pImageAction, &QAction::triggered, this, &ArticleEditor::onInsertImage);

    auto* pPageRefAction = pToolBar->addAction(QStringLiteral("页面引用"));
    pPageRefAction->setToolTip(QStringLiteral("插入图文攻略页面引用 ![[W:P]]"));
    connect(pPageRefAction, &QAction::triggered, this, &ArticleEditor::onInsertPageRef);

    // 工具栏放到顶部
    static_cast<QVBoxLayout*>(layout())->insertWidget(0, pToolBar);
}

void ArticleEditor::loadArticle(const Article& rArticle)
{
    m_bLoading = true;
    m_pSourceEdit->setPlainText(rArticle.strMarkdown);
    refreshPreview();
    m_bLoading = false;
    // 加载后重置防抖定时器，避免误触发 articleModified
    m_pDebounceTimer->stop();
}

void ArticleEditor::clear()
{
    m_bLoading = true;
    m_pSourceEdit->clear();
    m_pPreview->clear();
    m_bLoading = false;
    m_pDebounceTimer->stop();
}

void ArticleEditor::onSourceChanged()
{
    if(m_bLoading) {
        return;
    }
    m_pDebounceTimer->start();   // 重置防抖计时
}

void ArticleEditor::onDebounceTimeout()
{
    refreshPreview();
    emit articleModified(m_pSourceEdit->toPlainText());
}

void ArticleEditor::refreshPreview()
{
    m_pPreview->setMarkdownSource(m_pSourceEdit->toPlainText());
}

void ArticleEditor::insertMarkdownWrap(const QString& strBefore, const QString& strAfter)
{
    QTextCursor cursor = m_pSourceEdit->textCursor();
    if(cursor.hasSelection()) {
        // 选中文本：在前后包裹标记
        const QString strSelected = cursor.selectedText();
        cursor.insertText(strBefore + strSelected + strAfter);
    } else {
        // 无选中：插入标记并把光标放中间
        cursor.insertText(strBefore + strAfter);
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, strAfter.length());
        m_pSourceEdit->setTextCursor(cursor);
    }
}

void ArticleEditor::insertLinePrefix(const QString& strPrefix)
{
    QTextCursor cursor = m_pSourceEdit->textCursor();
    cursor.beginEditBlock();
    // 处理多行选中的情况：每行都加前缀
    int nStart = cursor.selectionStart();
    int nEnd = cursor.selectionEnd();
    cursor.setPosition(nStart);
    cursor.movePosition(QTextCursor::StartOfLine);
    while(cursor.position() <= nEnd) {
        cursor.insertText(strPrefix);
        if(!cursor.movePosition(QTextCursor::Down)) {
            break;
        }
        cursor.movePosition(QTextCursor::StartOfLine);
        // 更新 nEnd 以反映插入的字符
        nEnd += strPrefix.length();
    }
    cursor.endEditBlock();
}

void ArticleEditor::onInsertBold()
{
    insertMarkdownWrap(QStringLiteral("**"), QStringLiteral("**"));
}

void ArticleEditor::onInsertItalic()
{
    insertMarkdownWrap(QStringLiteral("*"), QStringLiteral("*"));
}

void ArticleEditor::onInsertHeading()
{
    insertLinePrefix(QStringLiteral("## "));
}

void ArticleEditor::onInsertList()
{
    insertLinePrefix(QStringLiteral("- "));
}

void ArticleEditor::onInsertLink()
{
    bool bOk = false;
    const QString strText = QInputDialog::getText(
        this, QStringLiteral("插入链接"), QStringLiteral("链接文字："),
        QLineEdit::Normal, QStringLiteral("链接文字"), &bOk);
    if(!bOk || strText.isEmpty()) {
        return;
    }
    const QString strUrl = QInputDialog::getText(
        this, QStringLiteral("插入链接"), QStringLiteral("链接地址："),
        QLineEdit::Normal, QStringLiteral("https://"), &bOk);
    if(!bOk || strUrl.isEmpty()) {
        return;
    }
    QTextCursor cursor = m_pSourceEdit->textCursor();
    cursor.insertText(QStringLiteral("[%1](%2)").arg(strText, strUrl));
}

void ArticleEditor::onInsertImage()
{
    const QString strFilePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择图片"), QString(),
        QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp *.gif)"));
    if(strFilePath.isEmpty()) {
        return;
    }

    QString strImagePath = strFilePath;
    // 有项目时复制图片到 assets/ 并改写路径为相对引用
    if(m_pProjectManager && m_pProjectManager->hasProject()) {
        const QString strAssetsDir = m_pProjectManager->projectDirectory()
                                     + QStringLiteral("/assets");
        QDir dir(strAssetsDir);
        if(!dir.exists()) {
            dir.mkpath(QStringLiteral("."));
        }
        const QFileInfo info(strFilePath);
        const QString strTarget = dir.filePath(
            QUuid::createUuid().toString(QUuid::WithoutBraces)
            + QLatin1Char('.') + info.suffix());
        if(QFile::copy(strFilePath, strTarget)) {
            strImagePath = QStringLiteral("assets/") + QFileInfo(strTarget).fileName();
        }
    }

    QTextCursor cursor = m_pSourceEdit->textCursor();
    cursor.insertText(QStringLiteral("![图片](%1)").arg(strImagePath));
}

void ArticleEditor::onInsertPageRef()
{
    // 第三期会实现图形化的页面选择器；本期用简单的文本输入
    bool bOk = false;
    const QString strRef = QInputDialog::getText(
        this, QStringLiteral("插入页面引用"),
        QStringLiteral("页面引用（格式 W:P，如 0:1 表示第0个攻略第1页）："),
        QLineEdit::Normal, QStringLiteral("0:0"), &bOk);
    if(!bOk || strRef.isEmpty()) {
        return;
    }
    // 验证格式
    const QStringList parts = strRef.split(QLatin1Char(':'));
    if(parts.size() != 2) {
        QMessageBox::warning(this, QStringLiteral("格式错误"),
                             QStringLiteral("请输入 W:P 格式，如 0:1"));
        return;
    }
    bool bWOk = false, bPOk = false;
    parts.at(0).toInt(&bWOk);
    parts.at(1).toInt(&bPOk);
    if(!bWOk || !bPOk) {
        QMessageBox::warning(this, QStringLiteral("格式错误"),
                             QStringLiteral("W 和 P 必须是数字"));
        return;
    }
    QTextCursor cursor = m_pSourceEdit->textCursor();
    cursor.insertText(QStringLiteral("![[%1]]").arg(strRef));
}

} // namespace bwm
