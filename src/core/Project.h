#pragma once

#include <QSize>
#include <QString>
#include <QVector>

namespace bwm {

// 攻略类型：对应规划第 5.5 节的内置六类模板。
// M1 仅定义枚举并参与序列化；模板系统在 M5 实现。
enum class WalkthroughType {
    Equipment,      // 装备推荐
    StatsCompare,   // 属性 / 数值对比
    StoryFlow,      // 剧情流程
    WeaponReview,   // 武器 / 角色评测
    MapPoints,      // 地图 / 点位
    Cover,          // 通用封面
    Custom,         // 自定义（未知类型的兜底）
};

// 序列化用的稳定字符串标识（JSON 中存字符串而非数字，保证可读与迁移友好）
QString walkthroughTypeToString(WalkthroughType type);
WalkthroughType walkthroughTypeFromString(const QString &text);

// 页面：一页 = 一张导出的 PNG。
// 组件列表在 M2 画布阶段加入，M1 仅保留名称与尺寸。
struct Page {
    QString name;
    QSize size; // 逻辑像素尺寸；导出时按倍率缩放
};

// 攻略：项目下的一个攻略，绑定一个类型（对应模板分类）。
struct Walkthrough {
    QString title;
    WalkthroughType type = WalkthroughType::Cover;
    QVector<Page> pages;
};

// 项目：对应一个游戏，磁盘上是一个自包含的 .bwm 文件夹。
struct Project {
    QString name;      // 游戏名
    QString filePath;  // project.json 的绝对路径
    QVector<Walkthrough> walkthroughs;
    // 预留：素材库（M3）、模板引用（M5）、主题引用（M5）
};

} // namespace bwm
