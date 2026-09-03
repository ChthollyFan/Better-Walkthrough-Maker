/**
 * @file MarkdownPreview.cpp
 * @author zhangweimu
 * @brief Markdown 预览控件实现。
 */
#include "app/panels/MarkdownPreview.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QUrl>

namespace bwm {

MarkdownPreview::MarkdownPreview(QWidget* pParent)
    : QTextBrowser(pParent)
{
    setOpenExternalLinks(true);
    setReadOnly(true);
}

void MarkdownPreview::setProjectDirectory(const QString& strDir)
{
    m_strProjectDirectory = strDir;
}

void MarkdownPreview::setMarkdownSource(const QString& strMarkdown)
{
    // QTextBrowser::setMarkdown 解析 CommonMark + GFM 并渲染为富文本
    document()->setMarkdown(strMarkdown);
}

QVariant MarkdownPreview::loadResource(int nType, const QUrl& rName)
{
    // 仅处理图片类型资源
    if(nType != QTextDocument::ImageResource) {
        return QTextBrowser::loadResource(nType, rName);
    }

    // 解析图片路径：支持相对路径（assets/xxx.png）和绝对路径
    QString strPath = rName.path();
    if(strPath.isEmpty()) {
        strPath = rName.toString();
    }

    QFileInfo info(strPath);
    // 相对路径：以项目目录为基准解析
    if(info.isRelative() && !m_strProjectDirectory.isEmpty()) {
        strPath = QDir(m_strProjectDirectory).filePath(strPath);
    }

    QFile file(strPath);
    if(file.open(QIODevice::ReadOnly)) {
        QImage image;
        if(image.loadFromData(file.readAll())) {
            return image;
        }
    }

    // 加载失败时回退到默认处理（显示占位）
    return QTextBrowser::loadResource(nType, rName);
}

} // namespace bwm
