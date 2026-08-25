#include "core/Project.h"

namespace bwm {

QString walkthroughTypeToString(WalkthroughType type)
{
    switch (type) {
    case WalkthroughType::Equipment:
        return QStringLiteral("equipment");
    case WalkthroughType::StatsCompare:
        return QStringLiteral("stats_compare");
    case WalkthroughType::StoryFlow:
        return QStringLiteral("story_flow");
    case WalkthroughType::WeaponReview:
        return QStringLiteral("weapon_review");
    case WalkthroughType::MapPoints:
        return QStringLiteral("map_points");
    case WalkthroughType::Cover:
        return QStringLiteral("cover");
    case WalkthroughType::Custom:
        return QStringLiteral("custom");
    }
    return QStringLiteral("custom");
}

WalkthroughType walkthroughTypeFromString(const QString &text)
{
    if (text == QStringLiteral("equipment"))
        return WalkthroughType::Equipment;
    if (text == QStringLiteral("stats_compare"))
        return WalkthroughType::StatsCompare;
    if (text == QStringLiteral("story_flow"))
        return WalkthroughType::StoryFlow;
    if (text == QStringLiteral("weapon_review"))
        return WalkthroughType::WeaponReview;
    if (text == QStringLiteral("map_points"))
        return WalkthroughType::MapPoints;
    if (text == QStringLiteral("cover"))
        return WalkthroughType::Cover;
    // 未知字符串按 Custom 处理，保证旧文件或手写文件可打开
    return WalkthroughType::Custom;
}

} // namespace bwm
