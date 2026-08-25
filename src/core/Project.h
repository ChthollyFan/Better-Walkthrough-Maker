/**
 * @file Project.h
 * @author zhangweimu
 * @brief 项目数据模型：项目 / 攻略 / 页面（M1 骨架版）。
 */
#ifndef BWM_CORE_PROJECT_H
#define BWM_CORE_PROJECT_H

#include <QSize>
#include <QString>
#include <QVector>

namespace bwm {

// 攻略类型：对应规划第 5.5 节内置六类模板（M1 仅定义枚举并参与序列化；模板系统 M5 实现）。
enum E_WALKTHROUGH_TYPE {
    E_WALKTHROUGH_TYPE_EQUIPMENT = 0,     // 装备推荐
    E_WALKTHROUGH_TYPE_STATS_COMPARE,     // 属性 / 数值对比
    E_WALKTHROUGH_TYPE_STORY_FLOW,        // 剧情流程
    E_WALKTHROUGH_TYPE_WEAPON_REVIEW,     // 武器 / 角色评测
    E_WALKTHROUGH_TYPE_MAP_POINTS,        // 地图 / 点位
    E_WALKTHROUGH_TYPE_COVER,             // 通用封面
    E_WALKTHROUGH_TYPE_CUSTOM,            // 自定义（未知类型兜底）
};

// 序列化用的稳定字符串标识（JSON 中存字符串而非数字，保证可读与迁移友好）
QString walkthroughTypeToString(E_WALKTHROUGH_TYPE eType);
E_WALKTHROUGH_TYPE walkthroughTypeFromString(const QString& strText);

// 页面：一页 = 一张导出的 PNG。组件列表在 M2 画布阶段加入，M1 仅保留名称与尺寸。
struct Page {
    QString strName;    // 页面名
    QSize size;         // 逻辑像素尺寸；导出时按倍率缩放
};

// 攻略：项目下的一个攻略，绑定一个类型（对应模板分类）。
struct Walkthrough {
    QString strTitle;                     // 攻略标题
    E_WALKTHROUGH_TYPE eType = E_WALKTHROUGH_TYPE_COVER;
    QVector<Page> vecPages;               // 页面列表
};

// 项目：对应一个游戏，磁盘上是一个自包含的 .bwm 文件夹。
struct Project {
    QString strName;                      // 游戏名
    QString strFilePath;                  // project.json 的绝对路径
    QVector<Walkthrough> vecWalkthroughs; // 攻略列表
    // 预留：素材库（M3）、模板引用（M5）、主题引用（M5）
};

} // namespace bwm

#endif // BWM_CORE_PROJECT_H
