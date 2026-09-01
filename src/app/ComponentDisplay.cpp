/**
 * @file ComponentDisplay.cpp
 * @author zhangweimu
 * @brief 组件显示辅助实现。
 */
#include "app/ComponentDisplay.h"

namespace bwm {

QString componentDisplayName(const Component& rComponent)
{
    switch(rComponent.eType) {
    case E_COMPONENT_TYPE_IMAGE:
        return QStringLiteral("图片");
    case E_COMPONENT_TYPE_TEXT:
        return rComponent.textData.strContent.left(12);
    case E_COMPONENT_TYPE_SHAPE:
        return QStringLiteral("形状");
    case E_COMPONENT_TYPE_TABLE:
        return QStringLiteral("表格");
    case E_COMPONENT_TYPE_STICKER:
        return QStringLiteral("贴纸");
    default:
        return QStringLiteral("组件");
    }
}

} // namespace bwm
