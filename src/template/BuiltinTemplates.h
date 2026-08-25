/**
 * @file BuiltinTemplates.h
 * @author zhangweimu
 * @brief 内置模板库：六类攻略模板（程序内生成布局）。
 */
#ifndef BWM_TEMPLATE_BUILTINTEMPLATES_H
#define BWM_TEMPLATE_BUILTINTEMPLATES_H

#include <QVector>

#include "core/Template.h"

namespace bwm {

// 返回全部内置模板（装备推荐/属性对比/剧情流程/武器评测/地图点位/通用封面）。
QVector<Template> builtinTemplates();

} // namespace bwm

#endif // BWM_TEMPLATE_BUILTINTEMPLATES_H
