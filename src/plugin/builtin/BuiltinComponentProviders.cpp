/**
 * @file BuiltinComponentProviders.cpp
 * @author zhangweimu
 * @brief 内置组件类型适配器实现。
 *
 * 各 Provider 的 createComponent / showInputDialog 逻辑迁移自原 MainWindow
 * 的 onAddImageComponent / onAddTextComponent / onAddShapeComponent /
 * onAddTableComponent / onAddStickerComponent 方法，行为保持一致。
 */
#include "plugin/builtin/BuiltinComponentProviders.h"

#include "settings/Settings.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QUuid>

namespace bwm {

// =========================================================================
// ImageComponentProvider
// =========================================================================

QString ImageComponentProvider::typeId() const
{
    return QStringLiteral("builtin.image");
}

QString ImageComponentProvider::displayName() const
{
    return QStringLiteral("图片");
}

QString ImageComponentProvider::menuPath() const
{
    return QStringLiteral("插入/图片");
}

Component ImageComponentProvider::createComponent(const PluginContext& rContext) const
{
    (void)rContext;
    Component component;
    component.eType = E_COMPONENT_TYPE_IMAGE;
    component.size = QSizeF(300, 200);
    return component;
}

bool ImageComponentProvider::requiresInputDialog() const
{
    return true;
}

bool ImageComponentProvider::showInputDialog(QWidget* pParent, Component& rComponent,
                                             const PluginContext& rContext) const
{
    // 弹出文件选择对话框
    const QString strFilePath = QFileDialog::getOpenFileName(
        pParent, QStringLiteral("选择图片"), QString(),
        QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp *.gif)"));
    if(strFilePath.isEmpty()) {
        return false;   // 用户取消
    }

    QString strAssetPath = strFilePath;
    // 优先复制进项目 assets/（自包含；有项目时才复制）
    if(!rContext.projectDirectory.isEmpty()) {
        const QString strAssetsDir = rContext.projectDirectory + QStringLiteral("/assets");
        QDir dir(strAssetsDir);
        if(!dir.exists()) {
            dir.mkpath(QStringLiteral("."));
        }
        const QFileInfo info(strFilePath);
        const QString strTarget = dir.filePath(
            QUuid::createUuid().toString(QUuid::WithoutBraces)
            + QLatin1Char('.') + info.suffix());
        if(QFile::copy(strFilePath, strTarget)) {
            strAssetPath = strTarget;
        }
    }

    rComponent.imageData.strFilePath = strAssetPath;
    return true;
}

// =========================================================================
// TextComponentProvider
// =========================================================================

QString TextComponentProvider::typeId() const
{
    return QStringLiteral("builtin.text");
}

QString TextComponentProvider::displayName() const
{
    return QStringLiteral("文本");
}

QString TextComponentProvider::menuPath() const
{
    return QStringLiteral("插入/文本");
}

Component TextComponentProvider::createComponent(const PluginContext& rContext) const
{
    Component component;
    component.eType = E_COMPONENT_TYPE_TEXT;
    component.textData.color = rContext.theme.textColor;
    component.size = QSizeF(300, 60);
    return component;
}

bool TextComponentProvider::requiresInputDialog() const
{
    return true;
}

bool TextComponentProvider::showInputDialog(QWidget* pParent, Component& rComponent,
                                            const PluginContext& rContext) const
{
    (void)rContext;
    bool bOk = false;
    const QString strContent = QInputDialog::getText(
        pParent, QStringLiteral("插入文本"), QStringLiteral("文本内容："),
        QLineEdit::Normal, QStringLiteral("攻略文本"), &bOk);
    if(!bOk) {
        return false;   // 用户取消
    }
    rComponent.textData.strContent = strContent;
    return true;
}

// =========================================================================
// ShapeComponentProvider
// =========================================================================

ShapeComponentProvider::ShapeComponentProvider(E_SHAPE_TYPE eShapeType, int nShapeType)
    : m_eShapeType(eShapeType)
    , m_nShapeType(nShapeType)
{
}

QString ShapeComponentProvider::typeId() const
{
    return QStringLiteral("builtin.shape.%1").arg(m_nShapeType);
}

QString ShapeComponentProvider::displayName() const
{
    switch(m_eShapeType) {
    case E_SHAPE_TYPE_RECTANGLE:
        return QStringLiteral("矩形");
    case E_SHAPE_TYPE_ROUND_RECT:
        return QStringLiteral("圆角矩形");
    case E_SHAPE_TYPE_ELLIPSE:
        return QStringLiteral("椭圆");
    case E_SHAPE_TYPE_LINE:
        return QStringLiteral("线条");
    default:
        return QStringLiteral("形状");
    }
}

QString ShapeComponentProvider::menuPath() const
{
    return QStringLiteral("插入/形状/%1").arg(displayName());
}

Component ShapeComponentProvider::createComponent(const PluginContext& rContext) const
{
    Component component;
    component.eType = E_COMPONENT_TYPE_SHAPE;
    component.shapeData.eShapeType = m_eShapeType;
    component.shapeData.fillColor = rContext.theme.secondaryColor;
    component.shapeData.borderColor = rContext.theme.primaryColor;
    component.size = QSizeF(200, 120);
    return component;
}

// =========================================================================
// TableComponentProvider
// =========================================================================

QString TableComponentProvider::typeId() const
{
    return QStringLiteral("builtin.table");
}

QString TableComponentProvider::displayName() const
{
    return QStringLiteral("表格");
}

QString TableComponentProvider::menuPath() const
{
    return QStringLiteral("插入/表格");
}

Component TableComponentProvider::createComponent(const PluginContext& rContext) const
{
    (void)rContext;
    Component component;
    component.eType = E_COMPONENT_TYPE_TABLE;
    component.size = QSizeF(420, 200);
    // 默认装备表模板数据（与原 MainWindow::onAddTableComponent 一致）
    component.tableData.vecRows = {
        {QStringLiteral("装备"), QStringLiteral("攻击"), QStringLiteral("获取途径")},
        {QStringLiteral("猎犬长牙"), QStringLiteral("145"), QStringLiteral("宁姆格福")},
        {QStringLiteral("名刀月隐"), QStringLiteral("160"), QStringLiteral("湖区")},
    };
    return component;
}

// =========================================================================
// StickerComponentProvider
// =========================================================================

StickerComponentProvider::StickerComponentProvider(E_STICKER_TYPE eStickerType, int nStickerType)
    : m_eStickerType(eStickerType)
    , m_nStickerType(nStickerType)
{
}

QString StickerComponentProvider::typeId() const
{
    return QStringLiteral("builtin.sticker.%1").arg(m_nStickerType);
}

QString StickerComponentProvider::displayName() const
{
    switch(m_eStickerType) {
    case E_STICKER_TYPE_TITLE_LINE:
        return QStringLiteral("标题装饰线");
    case E_STICKER_TYPE_CORNER_BADGE:
        return QStringLiteral("角标");
    case E_STICKER_TYPE_STAR_RATING:
        return QStringLiteral("推荐度星标");
    case E_STICKER_TYPE_ARROW:
        return QStringLiteral("箭头");
    case E_STICKER_TYPE_DIVIDER:
        return QStringLiteral("分割线");
    case E_STICKER_TYPE_CARD_BORDER:
        return QStringLiteral("卡片边框");
    default:
        return QStringLiteral("贴纸");
    }
}

QString StickerComponentProvider::menuPath() const
{
    return QStringLiteral("插入/装饰/%1").arg(displayName());
}

Component StickerComponentProvider::createComponent(const PluginContext& rContext) const
{
    Component component;
    component.eType = E_COMPONENT_TYPE_STICKER;
    component.stickerData.eStickerType = m_eStickerType;
    component.stickerData.color = rContext.theme.primaryColor;
    // 按贴纸类型给默认尺寸（与原 MainWindow::onAddStickerComponent 一致）
    switch(m_eStickerType) {
    case E_STICKER_TYPE_TITLE_LINE:
        component.size = QSizeF(320, 16);
        break;
    case E_STICKER_TYPE_CORNER_BADGE:
        component.size = QSizeF(60, 40);
        break;
    case E_STICKER_TYPE_STAR_RATING:
        component.size = QSizeF(160, 28);
        break;
    case E_STICKER_TYPE_ARROW:
        component.size = QSizeF(120, 40);
        break;
    case E_STICKER_TYPE_DIVIDER:
        component.size = QSizeF(320, 12);
        break;
    case E_STICKER_TYPE_CARD_BORDER:
    default:
        component.size = QSizeF(320, 180);
        break;
    }
    return component;
}

} // namespace bwm
