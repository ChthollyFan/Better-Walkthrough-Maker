/**
 * @file AssetPanel.cpp
 * @author zhangweimu
 * @brief 素材库面板实现。
 *
 * 逻辑迁移自原 MainWindow 的 refreshAssetList / onImportAssets /
 * onAssetDoubleClicked / onAssetContextMenu 方法。
 */
#include "app/panels/AssetPanel.h"

#include "project/ProjectManager.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QListWidget>
#include <QListWidgetItem>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QUuid>
#include <QVBoxLayout>

namespace bwm {

AssetPanel::AssetPanel(QWidget* pParent, ProjectManager* pProjectManager)
    : QWidget(pParent)
    , m_pProjectManager(pProjectManager)
{
    auto* pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);

    // 导入素材按钮
    QPushButton* pImportButton = new QPushButton(QStringLiteral("导入素材…"), this);
    connect(pImportButton, &QPushButton::clicked, this, &AssetPanel::onImportAssets);
    pLayout->addWidget(pImportButton);

    // 素材缩略图列表（图标模式）
    m_pList = new QListWidget(this);
    m_pList->setViewMode(QListView::IconMode);
    m_pList->setIconSize(QSize(56, 56));
    m_pList->setResizeMode(QListView::Adjust);
    m_pList->setSpacing(4);
    m_pList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pList, &QListWidget::itemDoubleClicked,
            this, &AssetPanel::onAssetDoubleClicked);
    connect(m_pList, &QListWidget::customContextMenuRequested,
            this, &AssetPanel::onAssetContextMenu);
    pLayout->addWidget(m_pList);
}

void AssetPanel::refreshAssetList()
{
    m_pList->clear();
    if(!m_pProjectManager->hasProject()) {
        return;
    }
    const QString strAssetsDir = m_pProjectManager->projectDirectory() + QStringLiteral("/assets");
    QDir dir(strAssetsDir);
    const QStringList filters = {
        QStringLiteral("*.png"), QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"),
        QStringLiteral("*.bmp"), QStringLiteral("*.webp"), QStringLiteral("*.gif")
    };
    const QStringList files = dir.entryList(filters, QDir::Files);
    for(const QString& strFile : files) {
        const QString strPath = dir.absoluteFilePath(strFile);
        const QImage image(strPath);
        auto* pItem = new QListWidgetItem(
            QIcon(QPixmap::fromImage(image.scaled(56, 56, Qt::KeepAspectRatio, Qt::SmoothTransformation))),
            strFile);
        pItem->setData(Qt::UserRole, strPath);
        pItem->setToolTip(strPath);
        m_pList->addItem(pItem);
    }
}

void AssetPanel::onImportAssets()
{
    if(!m_pProjectManager->hasProject()) {
        QMessageBox::information(this, QStringLiteral("导入素材"), QStringLiteral("请先打开项目"));
        return;
    }
    const QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("导入素材"), QString(),
        QStringLiteral("图片 (*.png *.jpg *.jpeg *.bmp *.webp *.gif)"));
    if(files.isEmpty()) {
        return;
    }
    const QString strAssetsDir = m_pProjectManager->projectDirectory() + QStringLiteral("/assets");
    QDir dir(strAssetsDir);
    if(!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    for(const QString& strSource : files) {
        const QFileInfo info(strSource);
        const QString strTarget = dir.filePath(QUuid::createUuid().toString(QUuid::WithoutBraces)
                                               + QLatin1Char('.') + info.suffix());
        QFile::copy(strSource, strTarget);
    }
    refreshAssetList();
    emit assetsChanged();
}

void AssetPanel::onAssetDoubleClicked(QListWidgetItem* pItem)
{
    if(!pItem) {
        return;
    }
    const QString strPath = pItem->data(Qt::UserRole).toString();
    if(strPath.isEmpty()) {
        return;
    }
    // 构造图片组件并发出信号，由 MainWindow 添加到画布
    Component component;
    component.eType = E_COMPONENT_TYPE_IMAGE;
    component.imageData.strFilePath = strPath;
    component.size = QSizeF(300, 200);
    emit assetInserted(component);
}

void AssetPanel::onAssetContextMenu(const QPoint& rPos)
{
    QListWidgetItem* pItem = m_pList->itemAt(rPos);
    if(!pItem) {
        return;
    }
    QMenu menu(this);
    QAction* pInsertAction = menu.addAction(QStringLiteral("插入到画布"));
    QAction* pDeleteAction = menu.addAction(QStringLiteral("删除素材"));
    QAction* pChosen = menu.exec(m_pList->viewport()->mapToGlobal(rPos));
    if(!pChosen) {
        return;
    }
    if(pChosen == pInsertAction) {
        onAssetDoubleClicked(pItem);
    } else if(pChosen == pDeleteAction) {
        const QString strPath = pItem->data(Qt::UserRole).toString();
        // 引用检查：若任一页面的图片组件引用该素材，禁止删除
        bool bInUse = false;
        const Project* pProject = m_pProjectManager->project();
        if(pProject) {
            for(const Walkthrough& rWalkthrough : pProject->vecWalkthroughs) {
                for(const Page& rPage : rWalkthrough.vecPages) {
                    for(const Component& rComponent : rPage.vecComponents) {
                        if(rComponent.eType == E_COMPONENT_TYPE_IMAGE
                           && rComponent.imageData.strFilePath == strPath) {
                            bInUse = true;
                            break;
                        }
                    }
                }
            }
        }
        if(bInUse) {
            QMessageBox::warning(this, QStringLiteral("删除素材"),
                                 QStringLiteral("该素材正被页面引用，无法删除"));
            return;
        }
        QFile::remove(strPath);
        refreshAssetList();
        emit assetsChanged();
    }
}

} // namespace bwm
