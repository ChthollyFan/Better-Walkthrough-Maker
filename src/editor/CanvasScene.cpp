/**
 * @file CanvasScene.cpp
 * @author zhangweimu
 * @brief 画布场景实现。
 */
#include "editor/CanvasScene.h"

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QUuid>

#include <algorithm>

#include "core/PageBackground.h"
#include "editor/ComponentItem.h"
#include "project/AssetStore.h"

namespace bwm {

namespace {

// 页面固定层序（数值大者绘制在上层）：
// 组件 zOrder 从 1 起算（见 addComponent），因此两者都必须在 0 以下。
// 注意：背景色矩形必须在背景图**之上**的先决条件是「无背景图」，
// 一旦有背景图，图片要盖住背景色——故图片 Z 值大于背景色矩形。
constexpr qreal dPageBackgroundColorZ = -2;   // 主题背景色矩形（最底层）
constexpr qreal dPageBackgroundImageZ = -1;   // 页面背景图（压在背景色之上、组件之下）

// 页面背景图图元：按「等比覆盖」绘制源图，不参与选择与编辑。
// 直接持有源图、在 paint 中裁剪缩放（不生成与页面等大的中间位图），
// 避免 4K 级背景图在画布上占用双份内存。
class PageBackgroundItem : public QGraphicsItem
{
public:
    PageBackgroundItem(const QImage& rImage, const QSizeF& rPageSize)
        : m_image(rImage)
        , m_pageSize(rPageSize)
    {
    }

    QRectF boundingRect() const override
    {
        return QRectF(QPointF(0, 0), m_pageSize);
    }

    void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override
    {
        (void)pOption;
        (void)pWidget;
        pPainter->setRenderHint(QPainter::SmoothPixmapTransform);
        PageBackground::paintCover(pPainter, m_image, boundingRect());
    }

private:
    QImage m_image;      // 背景图源图
    QSizeF m_pageSize;   // 页面尺寸（决定绘制区域）
};

} // namespace

CanvasScene::CanvasScene(QObject* pParent)
    : QGraphicsScene(pParent)
{
}

void CanvasScene::loadPage(const Page& rPage)
{
    clear();
    m_vecItems.clear();

    // 显式限定场景范围 = 页面尺寸，避免越界组件把场景撑大导致页面偏移
    setSceneRect(0, 0, rPage.size.width(), rPage.size.height());

    // 页面背景：主题背景色 + 灰边框，不可选中、永远在最底层（有背景图时被图片盖住）
    auto* pBackground = addRect(0, 0, rPage.size.width(), rPage.size.height(),
                                QPen(QColor(140, 140, 140)), QBrush(m_pageBackgroundColor));
    pBackground->setZValue(dPageBackgroundColorZ);
    pBackground->setFlag(QGraphicsItem::ItemIsSelectable, false);

    // 页面背景图：等比覆盖铺满整页，压在背景色之上、组件之下（不参与选中）
    // 加载失败时静默跳过，露出下面的主题背景色
    if (!rPage.strBackgroundImage.isEmpty()) {
        const QString strImagePath = AssetStore::resolvePath(rPage.strBackgroundImage,
                                                             m_strProjectDirectory);
        const QImage image(strImagePath);
        if (!image.isNull()) {
            auto* pBackgroundImage = new PageBackgroundItem(image, QSizeF(rPage.size));
            pBackgroundImage->setZValue(dPageBackgroundImageZ);
            pBackgroundImage->setFlag(QGraphicsItem::ItemIsSelectable, false);
            addItem(pBackgroundImage);
        }
    }

    rebuildItems(rPage.vecComponents);
}

void CanvasScene::clearPage()
{
    clear();
    m_vecItems.clear();
}

void CanvasScene::setPageBackgroundColor(const QColor& rColor)
{
    m_pageBackgroundColor = rColor;
    update();
}

void CanvasScene::setProjectDirectory(const QString& strDir)
{
    if (m_strProjectDirectory == strDir) {
        return;
    }
    m_strProjectDirectory = strDir;
    // 同步已有图元：卡片边框图片的相对路径需要项目目录才能解析
    for (ComponentItem* pItem : m_vecItems) {
        pItem->setProjectDirectory(strDir);
    }
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
    pItem->setProjectDirectory(m_strProjectDirectory);
    pItem->setZValue(component.nZOrder);
    connect(pItem, &ComponentItem::geometryChanged, this, &CanvasScene::componentsChanged);
    connect(pItem, &ComponentItem::editStarted, this, &CanvasScene::componentEditStarted);
    connect(pItem, &ComponentItem::editFinished, this, &CanvasScene::componentEditFinished);
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
        pItem->setProjectDirectory(m_strProjectDirectory);
        pItem->setZValue(rComponent.nZOrder);
        connect(pItem, &ComponentItem::geometryChanged, this, &CanvasScene::componentsChanged);
        connect(pItem, &ComponentItem::editStarted, this, &CanvasScene::componentEditStarted);
        connect(pItem, &ComponentItem::editFinished, this, &CanvasScene::componentEditFinished);
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

void CanvasScene::setSnapToGrid(bool bEnable)
{
    m_bSnapToGrid = bEnable;
}

void CanvasScene::setGridSize(int nSize)
{
    m_nGridSize = qBound(4, nSize, 100);
}

void CanvasScene::setSnapToGuides(bool bEnable)
{
    m_bSnapToGuides = bEnable;
}

QPointF CanvasScene::snapPoint(const QPointF& rPoint) const
{
    if (!m_bSnapToGrid) {
        return rPoint;
    }
    return QPointF(qRound(rPoint.x() / m_nGridSize) * m_nGridSize,
                   qRound(rPoint.y() / m_nGridSize) * m_nGridSize);
}

QPointF CanvasScene::snapRect(const QPointF& rTopLeft, const QSizeF& rSize, ComponentItem* pSelf) const
{
    // 先做网格吸附
    QPointF snapResult = snapPoint(rTopLeft);
    if (!m_bSnapToGuides) {
        return snapResult;
    }

    // 收集其他组件的候选对齐线（左/中/右、顶/中/底）
    QVector<qreal> vecCandidateX;
    QVector<qreal> vecCandidateY;
    for (const ComponentItem* pItem : m_vecItems) {
        if (pItem == pSelf) {
            continue;
        }
        const QPointF otherPos = pItem->component().pos;
        const QSizeF otherSize = pItem->component().size;
        vecCandidateX.append(otherPos.x());
        vecCandidateX.append(otherPos.x() + otherSize.width() / 2);
        vecCandidateX.append(otherPos.x() + otherSize.width());
        vecCandidateY.append(otherPos.y());
        vecCandidateY.append(otherPos.y() + otherSize.height() / 2);
        vecCandidateY.append(otherPos.y() + otherSize.height());
    }

    constexpr qreal dSnapThreshold = 6;
    const qreal rLeft = rTopLeft.x();
    const qreal rRight = rTopLeft.x() + rSize.width();
    const qreal rTop = rTopLeft.y();
    const qreal rBottom = rTopLeft.y() + rSize.height();

    for (const qreal dCandidateX : vecCandidateX) {
        for (const qreal dSideX : {rLeft, rTopLeft.x() + rSize.width() / 2, rRight}) {
            if (qAbs(dSideX - dCandidateX) < dSnapThreshold) {
                snapResult.rx() += dCandidateX - dSideX;
                break;
            }
        }
    }
    for (const qreal dCandidateY : vecCandidateY) {
        for (const qreal dSideY : {rTop, rTopLeft.y() + rSize.height() / 2, rBottom}) {
            if (qAbs(dSideY - dCandidateY) < dSnapThreshold) {
                snapResult.ry() += dCandidateY - dSideY;
                break;
            }
        }
    }
    return snapResult;
}

} // namespace bwm
