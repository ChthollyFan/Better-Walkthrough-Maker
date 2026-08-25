/**
 * @file Project.cpp
 * @author zhangweimu
 * @brief 项目数据模型的辅助函数实现（攻略类型字符串转换）。
 */
#include "core/Project.h"

namespace bwm {

QString walkthroughTypeToString(E_WALKTHROUGH_TYPE eType)
{
    switch (eType) {
    case E_WALKTHROUGH_TYPE_EQUIPMENT:
        return QStringLiteral("equipment");
    case E_WALKTHROUGH_TYPE_STATS_COMPARE:
        return QStringLiteral("stats_compare");
    case E_WALKTHROUGH_TYPE_STORY_FLOW:
        return QStringLiteral("story_flow");
    case E_WALKTHROUGH_TYPE_WEAPON_REVIEW:
        return QStringLiteral("weapon_review");
    case E_WALKTHROUGH_TYPE_MAP_POINTS:
        return QStringLiteral("map_points");
    case E_WALKTHROUGH_TYPE_COVER:
        return QStringLiteral("cover");
    case E_WALKTHROUGH_TYPE_CUSTOM:
        return QStringLiteral("custom");
    default:
        return QStringLiteral("custom");
    }
}

E_WALKTHROUGH_TYPE walkthroughTypeFromString(const QString& strText)
{
    if (strText == QStringLiteral("equipment")) {
        return E_WALKTHROUGH_TYPE_EQUIPMENT;
    }
    if (strText == QStringLiteral("stats_compare")) {
        return E_WALKTHROUGH_TYPE_STATS_COMPARE;
    }
    if (strText == QStringLiteral("story_flow")) {
        return E_WALKTHROUGH_TYPE_STORY_FLOW;
    }
    if (strText == QStringLiteral("weapon_review")) {
        return E_WALKTHROUGH_TYPE_WEAPON_REVIEW;
    }
    if (strText == QStringLiteral("map_points")) {
        return E_WALKTHROUGH_TYPE_MAP_POINTS;
    }
    if (strText == QStringLiteral("cover")) {
        return E_WALKTHROUGH_TYPE_COVER;
    }
    // 未知字符串按 Custom 处理，保证旧文件或手写文件可打开
    return E_WALKTHROUGH_TYPE_CUSTOM;
}

} // namespace bwm
