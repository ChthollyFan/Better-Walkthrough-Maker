/**
 * @file AssetPanel.cpp
 * @author zhangweimu
 * @brief 素材库面板实现。
 *
 * 逻辑迁移自原 MainWindow 的 refreshAssetList / onImportAssets /
 * onAssetDoubleClicked / onAssetContextMenu 方法。
 */
#include "app/panels/AssetPanel.h"

#include "project/AssetStore.h"
#include "project/ProjectManager.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QListWidget>
#include <QListWidgetItem>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace bwm {

namespace {

// 判断两个路径是否指向同一个文件（忽略分隔符与大小写差异，Windows 盘符大小写不敏感）
bool isSameFile(const QString& strLeft, const QString& strRight)
{
    return QDir::cleanPath(strLeft).compare(QDir::cleanPath(strRight),
                                            Qt::CaseInsensitive) == 0;
}

} // namespace

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
    // 复制逻辑统一走 AssetStore（与图片组件、页面背景图导入共用）
    const QString strProjectDir = m_pProjectManager->projectDirectory();
    for(const QString& strSource : files) {
        AssetStore::importImage(strSource, strProjectDir, nullptr);
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
        // 引用检查：若任一页面的图片组件或**页面背景图**引用该素材，禁止删除
        // （背景图存的是项目内相对路径，需先解析为绝对路径再比对）
        bool bInUse = false;
        const Project* pProject = m_pProjectManager->project();
        if(pProject) {
            const QString strProjectDir = m_pProjectManager->projectDirectory();
            for(const Walkthrough& rWalkthrough : pProject->vecWalkthroughs) {
                for(const Page& rPage : rWalkthrough.vecPages) {
                    if(!rPage.strBackgroundImage.isEmpty()
                       && isSameFile(AssetStore::resolvePath(rPage.strBackgroundImage, strProjectDir),
                                     strPath)) {
                        bInUse = true;
                        break;
                    }
                    for(const Component& rComponent : rPage.vecComponents) {
                        if(rComponent.eType == E_COMPONENT_TYPE_IMAGE
                           && isSameFile(rComponent.imageData.strFilePath, strPath)) {
                            bInUse = true;
                            break;
                        }
                    }
                    if(bInUse) {
                        break;
                    }
                }
                if(bInUse) {
                    break;
                }
            }
        }
        if(bInUse) {
            QMessageBox::warning(this, QStringLiteral("删除素材"),
                                 QStringLiteral("该素材正被页面（组件或页面背景图）引用，无法删除"));
            return;
        }
        QFile::remove(strPath);
        refreshAssetList();
        emit assetsChanged();
    }
}

} // namespace bwm
