/**
 * @file ComponentDisplay.h
 * @author zhangweimu
 * @brief 组件显示辅助：图层列表中组件的显示名称等 UI 辅助函数。
 *
 * 从原 MainWindow.cpp 匿名命名空间中提取，供 LayerPanel 复用。
 */
#ifndef BWM_APP_COMPONENTDISPLAY_H
#define BWM_APP_COMPONENTDISPLAY_H

#include <QString>

#include "core/Component.h"

namespace bwm {

/**
 * @brief 获取组件在图层列表中的显示名称。
 * @param rComponent  组件数据
 * @return            显示名称（图片→"图片"，文本→内容前12字符等）
 */
QString componentDisplayName(const Component& rComponent);

} // namespace bwm

#endif // BWM_APP_COMPONENTDISPLAY_H
