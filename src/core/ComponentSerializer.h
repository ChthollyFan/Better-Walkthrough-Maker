/**
 * @file ComponentSerializer.h
 * @author zhangweimu
 * @brief 组件与 JSON 的相互转换（project.json 与模板文件共用）。
 */
#ifndef BWM_CORE_COMPONENTSERIALIZER_H
#define BWM_CORE_COMPONENTSERIALIZER_H

#include "core/Component.h"

class QJsonObject;

namespace bwm {

// 组件序列化：项目文件与模板文件共用的组件 ↔ JSON 转换。
class ComponentSerializer {
public:
    static QJsonObject toJson(const Component& rComponent);
    static Component fromJson(const QJsonObject& rObject);
};

} // namespace bwm

#endif // BWM_CORE_COMPONENTSERIALIZER_H
