/**
 * @file CanvasScene.cpp
 * @author zhangweimu
 * @brief 画布场景实现。
 */
#include "editor/CanvasScene.h"

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QUuid>

#include <algorithm>

#include "editor/ComponentItem.h"

namespace bwm {

CanvasScene::CanvasScene(QObject* pParent)
    : QGraphicsScene(pParent)
{
}

void CanvasScene::loadPage(const Page& rPage)
{
    clear();
    m_vecItems.clear();

    // 页面背景：白底 + 灰边框，不可选中、永远在最底层
    auto* pBackground = addRect(0, 0, rPage.size.width(), rPage.size.height(),
                                QPen(QColor(140, 140, 140)), QBrush(Qt::white));
    pBackground->setZValue(-1);
    pBackground->setFlag(QGraphicsItem::ItemIsSelectable, false);

    rebuildItems(rPage.vecComponents);
}

void CanvasScene::syncToModel(Page* pPage)
{
    if (!pPage) {
        return;
    }
    QVector<Component> vecComponents;
    vecComponents.reserve(m_vecItems.size());
    for (const ComponentItem* pItem : m_vecItems) {
        vecComponents.append(pItem->component());
    }
    // 按 zOrder 排序，保证模型中的图层顺序一致
    std::sort(vecComponents.begin(), vecComponents.end(),
              [](const Component& rLeft, const Component& rRight) {
                  return rLeft.nZOrder < rRight.nZOrder;
              });
    pPage->vecComponents = vecComponents;
}

ComponentItem* CanvasScene::addComponent(const Component& rComponent)
{
    Component component = rComponent;
    if (component.strId.isEmpty()) {
        component.strId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    if (component.nZOrder <= 0) {
        int nMaxZ = 0;
        for (const ComponentItem* pItem : m_vecItems) {
            nMaxZ = qMax(nMaxZ, pItem->component().nZOrder);
        }
        component.nZOrder = nMaxZ + 1;
    }

    auto* pItem = new ComponentItem(component);
    pItem->setZValue(component.nZOrder);
    connect(pItem, &ComponentItem::geometryChanged, this, &CanvasScene::componentsChanged);
    addItem(pItem);
    m_vecItems.append(pItem);
    sortByZOrder();
    emit componentsChanged();
    return pItem;
}

void CanvasScene::removeSelectedComponents()
{
    bool bChanged = false;
    const QList<QGraphicsItem*> selected = selectedItems();
    for (QGraphicsItem* pGraphicsItem : selected) {
        auto* pItem = qgraphicsitem_cast<ComponentItem*>(pGraphicsItem);
        if (!pItem) {
            continue;
        }
        removeItem(pItem);
        m_vecItems.removeOne(pItem);
        delete pItem;
        bChanged = true;
    }
    sortByZOrder();
    if (bChanged) {
        emit componentsChanged();
    }
}

QVector<ComponentItem*> CanvasScene::selectedComponentItems() const
{
    QVector<ComponentItem*> vecResult;
    const QList<QGraphicsItem*> selected = selectedItems();
    for (QGraphicsItem* pItem : selected) {
        if (auto* pComponentItem = qgraphicsitem_cast<ComponentItem*>(pItem)) {
            vecResult.append(pComponentItem);
        }
    }
    return vecResult;
}

void CanvasScene::rebuildItems(const QVector<Component>& rComponents)
{
    for (const Component& rComponent : rComponents) {
        auto* pItem = new ComponentItem(rComponent);
        pItem->setZValue(rComponent.nZOrder);
        connect(pItem, &ComponentItem::geometryChanged, this, &CanvasScene::componentsChanged);
        addItem(pItem);
        m_vecItems.append(pItem);
    }
    sortByZOrder();
}

void CanvasScene::sortByZOrder()
{
    std::sort(m_vecItems.begin(), m_vecItems.end(),
              [](const ComponentItem* pLeft, const ComponentItem* pRight) {
                  return pLeft->component().nZOrder < pRight->component().nZOrder;
              });
    for (ComponentItem* pItem : m_vecItems) {
        pItem->setZValue(pItem->component().nZOrder);
    }
}

} // namespace bwm
