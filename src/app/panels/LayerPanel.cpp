/**
 * @file LayerPanel.cpp
 * @author zhangweimu
 * @brief 图层面板实现。
 *
 * 逻辑迁移自原 MainWindow 的 refreshLayerList / onCanvasSelectionChanged /
 * moveLayer / moveLayerTo / onLayerMoveUp / onLayerMoveDown / onLayerToTop /
 * onLayerToBottom / onLayerSelectionChanged / onLayerVisibilityChanged 方法。
 */
#include "app/panels/LayerPanel.h"

#include "app/ComponentDisplay.h"
#include "editor/CanvasScene.h"
#include "editor/CanvasView.h"
#include "editor/ComponentItem.h"

#include <climits>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace bwm {

// 图层列表项中保存 ComponentItem* 的 UserRole
constexpr int nComponentItemRole = Qt::UserRole + 1;

LayerPanel::LayerPanel(QWidget* pParent, CanvasScene* pScene, CanvasView* pView)
    : QWidget(pParent)
    , m_pScene(pScene)
    , m_pView(pView)
{
    auto* pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);

    m_pList = new QListWidget(this);
    m_pList->setMinimumWidth(180);
    connect(m_pList, &QListWidget::itemSelectionChanged,
            this, &LayerPanel::onSelectionChanged);
    connect(m_pList, &QListWidget::itemChanged,
            this, &LayerPanel::onVisibilityChanged);
    pLayout->addWidget(m_pList);

    // 图层操作按钮
    auto* pButtons = new QHBoxLayout;
    QPushButton* pMoveUpButton = new QPushButton(QStringLiteral("上移"), this);
    QPushButton* pMoveDownButton = new QPushButton(QStringLiteral("下移"), this);
    QPushButton* pToTopButton = new QPushButton(QStringLiteral("置顶"), this);
    QPushButton* pToBottomButton = new QPushButton(QStringLiteral("置底"), this);
    connect(pMoveUpButton, &QPushButton::clicked, this, &LayerPanel::onMoveUp);
    connect(pMoveDownButton, &QPushButton::clicked, this, &LayerPanel::onMoveDown);
    connect(pToTopButton, &QPushButton::clicked, this, &LayerPanel::onToTop);
    connect(pToBottomButton, &QPushButton::clicked, this, &LayerPanel::onToBottom);
    pButtons->addWidget(pMoveUpButton);
    pButtons->addWidget(pMoveDownButton);
    pButtons->addWidget(pToTopButton);
    pButtons->addWidget(pToBottomButton);
    pLayout->addLayout(pButtons);
}

void LayerPanel::refreshLayerList()
{
    m_bSyncing = true;
    m_pList->blockSignals(true);
    m_pList->clear();
    const QVector<ComponentItem*> vecItems = m_pScene->componentItems();
    // 列表行 0 在最底层（zOrder 最小），最后一行在最顶层
    for(const ComponentItem* pItem : vecItems) {
        const Component& rComponent = pItem->component();
        auto* pListItem = new QListWidgetItem(componentDisplayName(rComponent));
        pListItem->setFlags(pListItem->flags() | Qt::ItemIsUserCheckable);
        pListItem->setCheckState(rComponent.bVisible ? Qt::Checked : Qt::Unchecked);
        pListItem->setData(nComponentItemRole, QVariant::fromValue(static_cast<void*>(
            const_cast<ComponentItem*>(pItem))));
        m_pList->addItem(pListItem);
    }
    m_pList->blockSignals(false);
    m_bSyncing = false;
}

void LayerPanel::syncSelectionFromScene()
{
    if(m_bSyncing) {
        return;
    }
    m_bSyncing = true;
    const QVector<ComponentItem*> vecSelected = m_pScene->selectedComponentItems();
    for(int nRow = 0; nRow < m_pList->count(); ++nRow) {
        QListWidgetItem* pListItem = m_pList->item(nRow);
        auto* pItem = static_cast<ComponentItem*>(pListItem->data(nComponentItemRole).value<void*>());
        pListItem->setSelected(vecSelected.contains(pItem));
    }
    m_bSyncing = false;
}

void LayerPanel::moveLayer(int nOffset)
{
    const QList<QListWidgetItem*> selected = m_pList->selectedItems();
    if(selected.isEmpty()) {
        return;
    }
    const int nRow = m_pList->row(selected.first());
    const int nTarget = nRow + nOffset;
    if(nTarget < 0 || nTarget >= m_pList->count()) {
        return;
    }
    moveLayerTo(nTarget);
}

void LayerPanel::moveLayerTo(int nTargetIndex)
{
    const QList<QListWidgetItem*> selected = m_pList->selectedItems();
    if(selected.isEmpty() || m_pList->count() < 2) {
        return;
    }
    const int nSourceIndex = m_pList->row(selected.first());
    if(nSourceIndex == nTargetIndex) {
        return;
    }

    auto* pSourceItem = static_cast<ComponentItem*>(
        m_pList->item(nSourceIndex)->data(nComponentItemRole).value<void*>());
    auto* pTargetItem = static_cast<ComponentItem*>(
        m_pList->item(nTargetIndex)->data(nComponentItemRole).value<void*>());

    // 交换 zOrder 后刷新场景层级
    Component sourceComponent = pSourceItem->component();
    Component targetComponent = pTargetItem->component();
    const int nTemp = sourceComponent.nZOrder;
    sourceComponent.nZOrder = targetComponent.nZOrder;
    targetComponent.nZOrder = nTemp;
    pSourceItem->setComponent(sourceComponent);
    pTargetItem->setComponent(targetComponent);

    m_pScene->sortByZOrder();
    refreshLayerList();
    // 保持选中
    for(int nRow = 0; nRow < m_pList->count(); ++nRow) {
        if(m_pList->item(nRow)->data(nComponentItemRole).value<void*>() == pSourceItem) {
            m_pList->setCurrentRow(nRow);
            break;
        }
    }
    emit layerChanged();
}

void LayerPanel::onMoveUp()
{
    moveLayer(1);   // 行号增大 = 更上层
}

void LayerPanel::onMoveDown()
{
    moveLayer(-1);  // 行号减小 = 更下层
}

void LayerPanel::onToTop()
{
    moveLayerTo(m_pList->count() - 1);
}

void LayerPanel::onToBottom()
{
    moveLayerTo(0);
}

void LayerPanel::onSelectionChanged()
{
    if(m_bSyncing) {
        return;
    }
    const QList<QListWidgetItem*> selected = m_pList->selectedItems();
    if(selected.isEmpty()) {
        return;
    }
    auto* pItem = static_cast<ComponentItem*>(selected.first()->data(nComponentItemRole).value<void*>());
    if(pItem) {
        m_bSyncing = true;
        m_pScene->clearSelection();
        pItem->setSelected(true);
        m_bSyncing = false;
        m_pView->centerOn(pItem);
    }
}

void LayerPanel::onVisibilityChanged(QListWidgetItem* pListItem)
{
    if(m_bSyncing) {
        return;
    }
    auto* pItem = static_cast<ComponentItem*>(pListItem->data(nComponentItemRole).value<void*>());
    if(!pItem) {
        return;
    }
    Component component = pItem->component();
    component.bVisible = (pListItem->checkState() == Qt::Checked);
    pItem->setComponent(component);
    pItem->setVisible(component.bVisible);
    emit layerChanged();
}

} // namespace bwm
