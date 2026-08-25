/**
 * @file ComponentItem.cpp
 * @author zhangweimu
 * @brief 画布组件图元实现：渲染与交互（移动/缩放/旋转/双击编辑）。
 */
#include "editor/ComponentItem.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QCursor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGraphicsSceneMouseEvent>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSpinBox>
#include <QtMath>

#include "editor/CanvasScene.h"

namespace bwm {

namespace {

constexpr qreal dHandleSize = 8;          // 手柄边长
constexpr qreal dRotateHandleOffset = 24; // 旋转手柄距组件顶部的距离
constexpr qreal dMinSize = 8;             // 组件最小边长
constexpr qreal dHandleHitRadius = 6;     // 手柄命中半径

} // namespace

ComponentItem::ComponentItem(const Component& rComponent, QGraphicsItem* pParent)
    : QGraphicsObject(pParent)
    , m_component(rComponent)
{
    setFlags(ItemIsSelectable);
    setAcceptHoverEvents(true);
    setPos(rComponent.pos);
    setRotation(rComponent.dRotation);
    if (rComponent.eType == E_COMPONENT_TYPE_IMAGE && !rComponent.imageData.strFilePath.isEmpty()) {
        m_imageCache.load(rComponent.imageData.strFilePath);
    }
}

QRectF ComponentItem::boundingRect() const
{
    const QSizeF size = m_component.size;
    return QRectF(-dRotateHandleOffset - dHandleSize,
                  -dRotateHandleOffset - dHandleSize,
                  size.width() + 2 * (dRotateHandleOffset + dHandleSize),
                  size.height() + 2 * (dRotateHandleOffset + dHandleSize));
}

void ComponentItem::setComponent(const Component& rComponent)
{
    m_component = rComponent;
    setPos(rComponent.pos);
    setRotation(rComponent.dRotation);
    if (rComponent.eType == E_COMPONENT_TYPE_IMAGE
        && !rComponent.imageData.strFilePath.isEmpty()
        && m_imageCache.isNull()) {
        m_imageCache.load(rComponent.imageData.strFilePath);
    }
    update();
}

void ComponentItem::paint(QPainter* pPainter, const QStyleOptionGraphicsItem*, QWidget*)
{
    paintContent(pPainter);
    if (isSelected()) {
        paintSelectionDecoration(pPainter);
    }
}

void ComponentItem::paintContent(QPainter* pPainter)
{
    const QRectF contentRect(0, 0, m_component.size.width(), m_component.size.height());

    if (m_component.eType == E_COMPONENT_TYPE_IMAGE) {
        if (!m_imageCache.isNull()) {
            // 保持纵横比居中绘制
            const QSizeF scaled = m_imageCache.size().scaled(contentRect.size().toSize(), Qt::KeepAspectRatio);
            const QRectF targetRect((contentRect.width() - scaled.width()) / 2,
                                    (contentRect.height() - scaled.height()) / 2,
                                    scaled.width(), scaled.height());
            pPainter->drawImage(targetRect, m_imageCache);
        } else {
            pPainter->fillRect(contentRect, QColor(220, 220, 220));
            pPainter->drawText(contentRect, Qt::AlignCenter, QStringLiteral("图片加载失败"));
        }
        return;
    }

    if (m_component.eType == E_COMPONENT_TYPE_TEXT) {
        const TextData& rText = m_component.textData;
        QFont font(rText.strFontFamily.isEmpty() ? QStringLiteral("Microsoft YaHei") : rText.strFontFamily);
        font.setPixelSize(rText.nFontSize);
        font.setBold(rText.bBold);
        pPainter->setFont(font);
        pPainter->setPen(rText.color);
        pPainter->drawText(contentRect, rText.nAlign, rText.strContent);
        return;
    }

    // 形状
    const ShapeData& rShape = m_component.shapeData;
    QPainterPath path;
    switch (rShape.eShapeType) {
    case E_SHAPE_TYPE_ROUND_RECT:
        path.addRoundedRect(contentRect, 12, 12);
        break;
    case E_SHAPE_TYPE_ELLIPSE:
        path.addEllipse(contentRect);
        break;
    case E_SHAPE_TYPE_LINE: {
        pPainter->setPen(QPen(rShape.borderColor, rShape.nBorderWidth));
        pPainter->drawLine(QPointF(0, 0), QPointF(m_component.size.width(), m_component.size.height()));
        return;
    }
    case E_SHAPE_TYPE_RECTANGLE:
    default:
        path.addRect(contentRect);
        break;
    }
    pPainter->fillPath(path, rShape.fillColor);
    pPainter->strokePath(path, QPen(rShape.borderColor, rShape.nBorderWidth));
}

void ComponentItem::paintSelectionDecoration(QPainter* pPainter)
{
    const QRectF contentRect(0, 0, m_component.size.width(), m_component.size.height());
    pPainter->setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
    pPainter->setBrush(Qt::NoBrush);
    pPainter->drawRect(contentRect);

    pPainter->setBrush(Qt::white);
    for (int nHandle = E_HANDLE_TOP_LEFT; nHandle <= E_HANDLE_LEFT; ++nHandle) {
        pPainter->setPen(QPen(QColor(0, 120, 215), 1));
        pPainter->drawRect(handleRect(static_cast<E_HANDLE_TYPE>(nHandle)));
    }

    // 旋转手柄：连接线与圆点
    const QRectF rotateRect = handleRect(E_HANDLE_ROTATE);
    pPainter->setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
    pPainter->drawLine(QPointF(m_component.size.width() / 2, 0),
                       QPointF(rotateRect.center().x(), rotateRect.center().y() + rotateRect.height() / 2));
    pPainter->setBrush(QColor(0, 120, 215));
    pPainter->setPen(QPen(QColor(0, 120, 215), 1));
    pPainter->drawEllipse(rotateRect);
}

QRectF ComponentItem::handleRect(E_HANDLE_TYPE eHandle) const
{
    const qreal w = m_component.size.width();
    const qreal h = m_component.size.height();
    const qreal dHalf = dHandleSize / 2;
    QPointF center;

    switch (eHandle) {
    case E_HANDLE_TOP_LEFT:
        center = QPointF(0, 0);
        break;
    case E_HANDLE_TOP:
        center = QPointF(w / 2, 0);
        break;
    case E_HANDLE_TOP_RIGHT:
        center = QPointF(w, 0);
        break;
    case E_HANDLE_RIGHT:
        center = QPointF(w, h / 2);
        break;
    case E_HANDLE_BOTTOM_RIGHT:
        center = QPointF(w, h);
        break;
    case E_HANDLE_BOTTOM:
        center = QPointF(w / 2, h);
        break;
    case E_HANDLE_BOTTOM_LEFT:
        center = QPointF(0, h);
        break;
    case E_HANDLE_LEFT:
        center = QPointF(0, h / 2);
        break;
    case E_HANDLE_ROTATE:
        center = QPointF(w / 2, -dRotateHandleOffset);
        break;
    default:
        return QRectF();
    }
    return QRectF(center.x() - dHalf, center.y() - dHalf, dHandleSize, dHandleSize);
}

qreal ComponentItem::handleHitRadius() const
{
    return dHandleHitRadius;
}

ComponentItem::E_HANDLE_TYPE ComponentItem::hitTestHandle(const QPointF& rLocalPos) const
{
    if (!isSelected()) {
        return E_HANDLE_NONE;
    }
    // 旋转手柄优先级最高（与顶部手柄区域有重叠）
    if (handleRect(E_HANDLE_ROTATE).adjusted(-dHandleHitRadius, -dHandleHitRadius,
                                             dHandleHitRadius, dHandleHitRadius).contains(rLocalPos)) {
        return E_HANDLE_ROTATE;
    }
    for (int nHandle = E_HANDLE_TOP_LEFT; nHandle <= E_HANDLE_LEFT; ++nHandle) {
        const E_HANDLE_TYPE eHandle = static_cast<E_HANDLE_TYPE>(nHandle);
        if (handleRect(eHandle).adjusted(-dHandleHitRadius, -dHandleHitRadius,
                                         dHandleHitRadius, dHandleHitRadius).contains(rLocalPos)) {
            return eHandle;
        }
    }
    return E_HANDLE_NONE;
}

void ComponentItem::updateCursorByHandle(E_HANDLE_TYPE eHandle)
{
    switch (eHandle) {
    case E_HANDLE_TOP_LEFT:
    case E_HANDLE_BOTTOM_RIGHT:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case E_HANDLE_TOP_RIGHT:
    case E_HANDLE_BOTTOM_LEFT:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case E_HANDLE_TOP:
    case E_HANDLE_BOTTOM:
        setCursor(Qt::SizeVerCursor);
        break;
    case E_HANDLE_LEFT:
    case E_HANDLE_RIGHT:
        setCursor(Qt::SizeHorCursor);
        break;
    case E_HANDLE_ROTATE:
        setCursor(Qt::CrossCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

void ComponentItem::resizeByHandle(E_HANDLE_TYPE eHandle, const QPointF& rDelta)
{
    QPointF newPos = m_pressPos;
    QSizeF newSize = m_pressSize;

    const bool bLeft = (eHandle == E_HANDLE_TOP_LEFT || eHandle == E_HANDLE_BOTTOM_LEFT || eHandle == E_HANDLE_LEFT);
    const bool bRight = (eHandle == E_HANDLE_TOP_RIGHT || eHandle == E_HANDLE_BOTTOM_RIGHT || eHandle == E_HANDLE_RIGHT);
    const bool bTop = (eHandle == E_HANDLE_TOP_LEFT || eHandle == E_HANDLE_TOP_RIGHT || eHandle == E_HANDLE_TOP);
    const bool bBottom = (eHandle == E_HANDLE_BOTTOM_LEFT || eHandle == E_HANDLE_BOTTOM_RIGHT || eHandle == E_HANDLE_BOTTOM);

    if (bLeft) {
        newPos.rx() += rDelta.x();
        newSize.rwidth() -= rDelta.x();
        if (newSize.width() < dMinSize) {
            newPos.rx() = m_pressPos.x() + m_pressSize.width() - dMinSize;
            newSize.rwidth() = dMinSize;
        }
    }
    if (bRight) {
        newSize.rwidth() += rDelta.x();
        if (newSize.width() < dMinSize) {
            newSize.rwidth() = dMinSize;
        }
    }
    if (bTop) {
        newPos.ry() += rDelta.y();
        newSize.rheight() -= rDelta.y();
        if (newSize.height() < dMinSize) {
            newPos.ry() = m_pressPos.y() + m_pressSize.height() - dMinSize;
            newSize.rheight() = dMinSize;
        }
    }
    if (bBottom) {
        newSize.rheight() += rDelta.y();
        if (newSize.height() < dMinSize) {
            newSize.rheight() = dMinSize;
        }
    }

    m_component.pos = newPos;
    m_component.size = newSize;
    if (auto* pScene = qobject_cast<CanvasScene*>(scene())) {
        m_component.pos = pScene->snapRect(m_component.pos, m_component.size, this);
    }
    setPos(m_component.pos);
    prepareGeometryChange();
    update();
    emit geometryChanged();
}

void ComponentItem::updateRotateByMouse(const QPointF& rScenePos)
{
    const QPointF centerScene = mapToScene(QPointF(m_component.size.width() / 2, m_component.size.height() / 2));
    const qreal dAngle = qRadiansToDegrees(std::atan2(rScenePos.y() - centerScene.y(),
                                                      rScenePos.x() - centerScene.x()));
    m_component.dRotation = dAngle - m_dRotateStartAngle;
    setRotation(m_component.dRotation);
    update();
    emit geometryChanged();
}

void ComponentItem::mousePressEvent(QGraphicsSceneMouseEvent* pEvent)
{
    if (m_component.bLocked) {
        pEvent->ignore();
        return;
    }
    const QPointF localPos = pEvent->pos();
    m_eActiveHandle = hitTestHandle(localPos);
    m_pressPos = m_component.pos;
    m_pressSize = m_component.size;
    m_pressMouseScene = pEvent->scenePos();
    emit editStarted();

    if (m_eActiveHandle == E_HANDLE_ROTATE) {
        const QPointF centerScene = mapToScene(QPointF(m_component.size.width() / 2, m_component.size.height() / 2));
        m_dRotateStartAngle = qRadiansToDegrees(std::atan2(m_pressMouseScene.y() - centerScene.y(),
                                                           m_pressMouseScene.x() - centerScene.x()))
            - m_component.dRotation;
        return;
    }
    if (m_eActiveHandle == E_HANDLE_NONE) {
        // 组件内部：允许移动（不调用基类，避免与手柄处理冲突）
        m_pressMouseLocal = localPos;
        setCursor(Qt::ClosedHandCursor);
        // 多选联动：记录其余选中组件的起始位置
        m_vecDragItems.clear();
        m_vecDragStartPos.clear();
        if (isSelected()) {
            if (auto* pScene = qobject_cast<CanvasScene*>(scene())) {
                const QVector<ComponentItem*> vecSelected = pScene->selectedComponentItems();
                for (ComponentItem* pOther : vecSelected) {
                    if (pOther != this) {
                        m_vecDragItems.append(pOther);
                        m_vecDragStartPos.append(pOther->component().pos);
                    }
                }
            }
        }
        return;
    }
    // 命中缩放手柄：记录，开始缩放
    pEvent->accept();
}

void ComponentItem::mouseMoveEvent(QGraphicsSceneMouseEvent* pEvent)
{
    const QPointF delta = pEvent->scenePos() - m_pressMouseScene;

    if (m_eActiveHandle == E_HANDLE_ROTATE) {
        updateRotateByMouse(pEvent->scenePos());
        return;
    }
    if (m_eActiveHandle == E_HANDLE_NONE) {
        // 移动：先对整体位移做吸附，保证多选组件的相对位置不变
        QPointF snappedDelta = delta;
        if (auto* pScene = qobject_cast<CanvasScene*>(scene())) {
            const QPointF snappedPos = pScene->snapRect(m_pressPos + delta, m_component.size, this);
            snappedDelta = snappedPos - m_pressPos;
        }
        m_component.pos = m_pressPos + snappedDelta;
        setPos(m_component.pos);
        // 联动移动其余选中组件（使用同一吸附后的位移）
        for (int nIndex = 0; nIndex < m_vecDragItems.size(); ++nIndex) {
            ComponentItem* pOther = m_vecDragItems.at(nIndex);
            Component otherComponent = pOther->component();
            otherComponent.pos = m_vecDragStartPos.at(nIndex) + snappedDelta;
            pOther->setComponent(otherComponent);
            emit pOther->geometryChanged();
        }
        update();
        emit geometryChanged();
        return;
    }
    // 缩放
    resizeByHandle(m_eActiveHandle, delta);
}

void ComponentItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* pEvent)
{
    m_eActiveHandle = E_HANDLE_NONE;
    m_vecDragItems.clear();
    m_vecDragStartPos.clear();
    setCursor(Qt::ArrowCursor);
    emit editFinished();
    QGraphicsObject::mouseReleaseEvent(pEvent);
}

void ComponentItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* pEvent)
{
    editContent();
    QGraphicsObject::mouseDoubleClickEvent(pEvent);
}

void ComponentItem::editContent()
{
    if (m_component.eType != E_COMPONENT_TYPE_TEXT) {
        return;
    }

    emit editStarted();

    // 文本样式编辑对话框：内容 / 字号 / 颜色 / 加粗 / 对齐
    QDialog dialog;
    dialog.setWindowTitle(QStringLiteral("编辑文本"));
    auto* pFormLayout = new QFormLayout(&dialog);

    auto* pContentEdit = new QLineEdit(&dialog);
    pContentEdit->setText(m_component.textData.strContent);
    pFormLayout->addRow(QStringLiteral("内容："), pContentEdit);

    auto* pFontSizeSpin = new QSpinBox(&dialog);
    pFontSizeSpin->setRange(6, 400);
    pFontSizeSpin->setValue(m_component.textData.nFontSize);
    pFormLayout->addRow(QStringLiteral("字号："), pFontSizeSpin);

    auto* pBoldCheck = new QCheckBox(QStringLiteral("加粗"), &dialog);
    pBoldCheck->setChecked(m_component.textData.bBold);
    pFormLayout->addRow(QStringLiteral("样式："), pBoldCheck);

    auto* pAlignCombo = new QComboBox(&dialog);
    pAlignCombo->addItem(QStringLiteral("左对齐"), int(Qt::AlignLeft | Qt::AlignVCenter));
    pAlignCombo->addItem(QStringLiteral("居中"), int(Qt::AlignHCenter | Qt::AlignVCenter));
    pAlignCombo->addItem(QStringLiteral("右对齐"), int(Qt::AlignRight | Qt::AlignVCenter));
    pAlignCombo->addItem(QStringLiteral("两端对齐"), int(Qt::AlignJustify | Qt::AlignVCenter));
    const int nCurrentAlign = m_component.textData.nAlign;
    int nAlignIndex = 0;
    for (int nIndex = 0; nIndex < pAlignCombo->count(); ++nIndex) {
        if (pAlignCombo->itemData(nIndex).toInt() == nCurrentAlign) {
            nAlignIndex = nIndex;
            break;
        }
    }
    pAlignCombo->setCurrentIndex(nAlignIndex);
    pFormLayout->addRow(QStringLiteral("对齐："), pAlignCombo);

    auto* pColorButton = new QPushButton(&dialog);
    QColor color = m_component.textData.color;
    pColorButton->setStyleSheet(QStringLiteral("background-color: %1;").arg(color.name()));
    const auto updateColorButton = [pColorButton](const QColor& rColor) {
        pColorButton->setStyleSheet(QStringLiteral("background-color: %1;").arg(rColor.name()));
    };
    connect(pColorButton, &QPushButton::clicked, &dialog, [pColorButton, &color, updateColorButton]() {
        const QColor chosen = QColorDialog::getColor(color, pColorButton, QStringLiteral("选择文字颜色"));
        if (chosen.isValid()) {
            color = chosen;
            updateColorButton(chosen);
        }
    });
    pFormLayout->addRow(QStringLiteral("颜色："), pColorButton);

    auto* pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    pButtons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    pButtons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(pButtons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(pButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    pFormLayout->addRow(pButtons);

    if (dialog.exec() == QDialog::Accepted) {
        m_component.textData.strContent = pContentEdit->text();
        m_component.textData.nFontSize = pFontSizeSpin->value();
        m_component.textData.bBold = pBoldCheck->isChecked();
        m_component.textData.nAlign = pAlignCombo->currentData().toInt();
        m_component.textData.color = color;
        prepareGeometryChange();
        update();
        emit geometryChanged();
    }

    emit editFinished();
}

void ComponentItem::hoverMoveEvent(QGraphicsSceneHoverEvent* pEvent)
{
    updateCursorByHandle(hitTestHandle(pEvent->pos()));
    QGraphicsObject::hoverMoveEvent(pEvent);
}

void ComponentItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* pEvent)
{
    setCursor(Qt::ArrowCursor);
    QGraphicsObject::hoverLeaveEvent(pEvent);
}

} // namespace bwm
