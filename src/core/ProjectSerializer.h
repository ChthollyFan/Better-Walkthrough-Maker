#pragma once

#include "core/Project.h"

#include <QString>

namespace bwm {

// project.json 的读写与容错解析。
// 设计要点（规划第 4.1 节）：
// - 纯 JSON、可读；文件头带 formatVersion 字段，为格式迁移预留。
// - 缺失字段取默认值；未知攻略类型取 Custom；版本高于当前则拒绝打开。
class ProjectSerializer {
public:
    static constexpr int kCurrentFormatVersion = 1;

    // 项目 → JSON 文本
    static QString toJson(const Project &project);

    // JSON 文本 → 项目。成功返回 true；失败返回 false 并给出中文原因。
    static bool fromJson(const QString &json, Project *project, QString *errorMessage);
};

} // namespace bwm
