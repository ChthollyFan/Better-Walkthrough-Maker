/**
 * @file Template.h
 * @author zhangweimu
 * @brief 模板结构与序列化（按攻略保存的多页模板，.json 可分享）。
 */
#ifndef BWM_CORE_TEMPLATE_H
#define BWM_CORE_TEMPLATE_H

#include <QString>
#include <QVector>

#include "core/Project.h"

namespace bwm {

// 模板：一个攻略的完整布局（标题/类型/说明 + 全部页面），用于快速新建攻略。
struct Template {
    QString strName;                        // 模板名
    E_WALKTHROUGH_TYPE eType = E_WALKTHROUGH_TYPE_COVER;   // 攻略类型
    QString strDescription;                 // 说明
    QVector<Page> vecPages;                 // 页面布局
};

// 模板文件（.json）的读写。
class TemplateSerializer {
public:
    static constexpr int nCurrentFormatVersion = 1;

    static QString toJson(const Template& rTemplate);
    static bool fromJson(const QString& rJson, Template* pTemplate, QString* pErrorMessage);
    static bool writeFile(const Template& rTemplate, const QString& strFilePath, QString* pErrorMessage);
    static bool readFile(const QString& strFilePath, Template* pTemplate, QString* pErrorMessage);
};

} // namespace bwm

#endif // BWM_CORE_TEMPLATE_H
