/**
 * @file BuiltinTemplates.cpp
 * @author zhangweimu
 * @brief 内置模板布局生成：每种攻略类型一个模板页（1080×1440 竖版）。
 */
#include "template/BuiltinTemplates.h"

namespace bwm {

namespace {

const QSize kPageSize(1080, 1440);
const QColor kTextColor(33, 33, 33);
const QColor kPrimaryColor(230, 57, 70);
const QColor kCardFill(245, 245, 245);
const QColor kCardBorder(200, 200, 200);

Component makeText(const QString& rContent, const QPointF& rPos, const QSizeF& rSize,
                   int nFontSize, bool bBold = false, const QColor& rColor = kTextColor)
{
    Component component;
    component.eType = E_COMPONENT_TYPE_TEXT;
    component.pos = rPos;
    component.size = rSize;
    component.textData.strContent = rContent;
    component.textData.nFontSize = nFontSize;
    component.textData.bBold = bBold;
    component.textData.color = rColor;
    component.textData.nAlign = Qt::AlignLeft | Qt::AlignVCenter;
    return component;
}

Component makeShape(E_SHAPE_TYPE eShapeType, const QPointF& rPos, const QSizeF& rSize,
                    const QColor& rFill, const QColor& rBorder = kCardBorder, int nBorderWidth = 1)
{
    Component component;
    component.eType = E_COMPONENT_TYPE_SHAPE;
    component.pos = rPos;
    component.size = rSize;
    component.shapeData.eShapeType = eShapeType;
    component.shapeData.fillColor = rFill;
    component.shapeData.borderColor = rBorder;
    component.shapeData.nBorderWidth = nBorderWidth;
    return component;
}

Component makeSticker(E_STICKER_TYPE eStickerType, const QPointF& rPos, const QSizeF& rSize,
                      const QColor& rColor = kPrimaryColor)
{
    Component component;
    component.eType = E_COMPONENT_TYPE_STICKER;
    component.pos = rPos;
    component.size = rSize;
    component.stickerData.eStickerType = eStickerType;
    component.stickerData.color = rColor;
    return component;
}

Component makeTable(const QVector<QStringList>& rRows, const QPointF& rPos, const QSizeF& rSize)
{
    Component component;
    component.eType = E_COMPONENT_TYPE_TABLE;
    component.pos = rPos;
    component.size = rSize;
    component.tableData.vecRows = rRows;
    return component;
}

// 统一给组件分配递增 zOrder
void assignZOrder(QVector<Component>& rComponents)
{
    for (int nIndex = 0; nIndex < rComponents.size(); ++nIndex) {
        rComponents[nIndex].nZOrder = nIndex;
    }
}

// 组装一个标准页
Page makePage(const QVector<Component>& rComponents, const QString& rName = QStringLiteral("页面 1"))
{
    QVector<Component> components = rComponents;
    assignZOrder(components);
    Page page;
    page.strName = rName;
    page.size = kPageSize;
    page.vecComponents = components;
    return page;
}

Template makeTemplate(const QString& rName, E_WALKTHROUGH_TYPE eType,
                      const QString& rDescription, const QVector<Component>& rComponents)
{
    Template t;
    t.strName = rName;
    t.eType = eType;
    t.strDescription = rDescription;
    t.vecPages.append(makePage(rComponents));
    return t;
}

// 装备推荐：标题 + 装饰线 + 三张装备卡片
QVector<Component> equipmentComponents()
{
    QVector<Component> components;
    components.append(makeText(QStringLiteral("装备推荐"), QPointF(60, 60), QSizeF(600, 80), 48, true));
    components.append(makeSticker(E_STICKER_TYPE_TITLE_LINE, QPointF(60, 150), QSizeF(400, 16)));
    const QVector<QPointF> cardPos = {QPointF(60, 220), QPointF(400, 220), QPointF(740, 220)};
    const QVector<QString> cardNames = {QStringLiteral("武器名 A"), QStringLiteral("武器名 B"), QStringLiteral("武器名 C")};
    for (int nIndex = 0; nIndex < cardPos.size(); ++nIndex) {
        components.append(makeShape(E_SHAPE_TYPE_ROUND_RECT, cardPos.at(nIndex),
                                    QSizeF(280, 320), kCardFill, kCardBorder, 1));
        components.append(makeShape(E_SHAPE_TYPE_ROUND_RECT,
                                    QPointF(cardPos.at(nIndex).x() + 30, cardPos.at(nIndex).y() + 30),
                                    QSizeF(220, 160), QColor(220, 220, 220)));
        components.append(makeText(QStringLiteral("装备图片位"), QPointF(cardPos.at(nIndex).x() + 30, cardPos.at(nIndex).y() + 80),
                                   QSizeF(220, 60), 22, false));
        components.append(makeText(cardNames.at(nIndex), QPointF(cardPos.at(nIndex).x() + 20, cardPos.at(nIndex).y() + 210),
                                   QSizeF(240, 40), 28, true));
        components.append(makeText(QStringLiteral("获取途径：待填写"), QPointF(cardPos.at(nIndex).x() + 20, cardPos.at(nIndex).y() + 255),
                                   QSizeF(240, 36), 18));
        components.append(makeSticker(E_STICKER_TYPE_STAR_RATING, QPointF(cardPos.at(nIndex).x() + 20, cardPos.at(nIndex).y() + 290),
                                      QSizeF(120, 20), kPrimaryColor));
    }
    return components;
}

// 属性/数值对比：标题 + 对比表格
QVector<Component> statsComponents()
{
    QVector<Component> components;
    components.append(makeText(QStringLiteral("属性对比"), QPointF(60, 60), QSizeF(600, 80), 48, true));
    components.append(makeSticker(E_STICKER_TYPE_TITLE_LINE, QPointF(60, 150), QSizeF(400, 16)));
    components.append(makeTable({
        {QStringLiteral("属性"), QStringLiteral("方案 A"), QStringLiteral("方案 B")},
        {QStringLiteral("攻击力"), QStringLiteral("120"), QStringLiteral("145")},
        {QStringLiteral("防御力"), QStringLiteral("80"), QStringLiteral("70")},
        {QStringLiteral("负重"), QStringLiteral("轻"), QStringLiteral("中")},
    }, QPointF(60, 240), QSizeF(960, 360)));
    components.append(makeText(QStringLiteral("说明：双击表格可编辑数据，或从剪贴板导入 CSV"),
                               QPointF(60, 640), QSizeF(900, 40), 20));
    return components;
}

// 剧情流程：标题 + 时间线步骤
QVector<Component> storyComponents()
{
    QVector<Component> components;
    components.append(makeText(QStringLiteral("剧情流程"), QPointF(60, 60), QSizeF(600, 80), 48, true));
    components.append(makeSticker(E_STICKER_TYPE_TITLE_LINE, QPointF(60, 150), QSizeF(400, 16)));
    // 时间线：竖线 + 步骤点 + 步骤文本
    components.append(makeShape(E_SHAPE_TYPE_RECTANGLE, QPointF(100, 240), QSizeF(6, 800), kPrimaryColor));
    const QVector<QString> steps = {
        QStringLiteral("第一步：接取任务"),
        QStringLiteral("第二步：击败首领"),
        QStringLiteral("第三步：解锁区域"),
        QStringLiteral("第四步：完成结局"),
    };
    for (int nIndex = 0; nIndex < steps.size(); ++nIndex) {
        const qreal dY = 240 + nIndex * 200;
        components.append(makeShape(E_SHAPE_TYPE_ELLIPSE, QPointF(84, dY), QSizeF(38, 38), kPrimaryColor));
        components.append(makeText(QStringLiteral("%1").arg(nIndex + 1), QPointF(94, dY + 4), QSizeF(20, 30), 20, true, QColor(Qt::white)));
        components.append(makeText(steps.at(nIndex), QPointF(160, dY), QSizeF(700, 40), 26, nIndex == 0));
        components.append(makeText(QStringLiteral("详情描述待填写"), QPointF(160, dY + 45), QSizeF(700, 32), 18));
    }
    return components;
}

// 武器/角色评测：标题 + 大图位 + 星标 + 优缺点
QVector<Component> reviewComponents()
{
    QVector<Component> components;
    components.append(makeText(QStringLiteral("武器评测"), QPointF(60, 60), QSizeF(600, 80), 48, true));
    components.append(makeSticker(E_STICKER_TYPE_TITLE_LINE, QPointF(60, 150), QSizeF(400, 16)));
    components.append(makeShape(E_SHAPE_TYPE_ROUND_RECT, QPointF(60, 220), QSizeF(600, 400), QColor(230, 230, 230)));
    components.append(makeText(QStringLiteral("武器图片位"), QPointF(60, 380), QSizeF(600, 60), 24));
    components.append(makeSticker(E_STICKER_TYPE_STAR_RATING, QPointF(60, 660), QSizeF(260, 32), kPrimaryColor));
    components.append(makeText(QStringLiteral("综合评分：★★★★★"), QPointF(60, 700), QSizeF(400, 36), 24));
    components.append(makeText(QStringLiteral("优点："), QPointF(60, 780), QSizeF(200, 36), 28, true));
    components.append(makeText(QStringLiteral("· 优点一待填写"), QPointF(100, 830), QSizeF(800, 34), 20));
    components.append(makeText(QStringLiteral("· 优点二待填写"), QPointF(100, 880), QSizeF(800, 34), 20));
    components.append(makeText(QStringLiteral("缺点："), QPointF(60, 960), QSizeF(200, 36), 28, true));
    components.append(makeText(QStringLiteral("· 缺点一待填写"), QPointF(100, 1010), QSizeF(800, 34), 20));
    return components;
}

// 地图/点位：背景图位 + 标注点
QVector<Component> mapComponents()
{
    QVector<Component> components;
    components.append(makeText(QStringLiteral("地图点位"), QPointF(60, 60), QSizeF(600, 80), 48, true));
    components.append(makeSticker(E_STICKER_TYPE_TITLE_LINE, QPointF(60, 150), QSizeF(400, 16)));
    components.append(makeShape(E_SHAPE_TYPE_RECTANGLE, QPointF(60, 220), QSizeF(960, 900), QColor(235, 235, 235)));
    components.append(makeText(QStringLiteral("地图图片位"), QPointF(60, 620), QSizeF(960, 60), 24));
    const QVector<QPointF> pointPos = {QPointF(300, 420), QPointF(600, 700), QPointF(800, 380)};
    const QVector<QString> pointNames = {QStringLiteral("点位 A"), QStringLiteral("点位 B"), QStringLiteral("点位 C")};
    for (int nIndex = 0; nIndex < pointPos.size(); ++nIndex) {
        components.append(makeShape(E_SHAPE_TYPE_ELLIPSE, pointPos.at(nIndex), QSizeF(40, 40), kPrimaryColor));
        components.append(makeText(pointNames.at(nIndex), QPointF(pointPos.at(nIndex).x() + 48, pointPos.at(nIndex).y() - 5),
                                   QSizeF(200, 36), 22, true));
    }
    return components;
}

// 通用封面：大标题 + 副标题 + 配图位 + 署名位
QVector<Component> coverComponents()
{
    QVector<Component> components;
    components.append(makeText(QStringLiteral("攻略标题"), QPointF(120, 200), QSizeF(840, 110), 60, true));
    components.append(makeSticker(E_STICKER_TYPE_TITLE_LINE, QPointF(120, 330), QSizeF(360, 16)));
    components.append(makeText(QStringLiteral("副标题 / 版本说明"), QPointF(120, 370), QSizeF(840, 50), 28));
    components.append(makeShape(E_SHAPE_TYPE_ROUND_RECT, QPointF(140, 480), QSizeF(800, 560), QColor(235, 235, 235)));
    components.append(makeText(QStringLiteral("封面图片位"), QPointF(140, 730), QSizeF(800, 60), 26));
    components.append(makeSticker(E_STICKER_TYPE_DIVIDER, QPointF(120, 1140), QSizeF(840, 12), kPrimaryColor));
    components.append(makeText(QStringLiteral("作者：你的名字"), QPointF(120, 1200), QSizeF(400, 40), 22));
    components.append(makeText(QStringLiteral("更新日期：2026-XX-XX"), QPointF(120, 1250), QSizeF(400, 36), 18));
    return components;
}

} // namespace

QVector<Template> builtinTemplates()
{
    QVector<Template> templates;
    templates.append(makeTemplate(QStringLiteral("装备推荐"), E_WALKTHROUGH_TYPE_EQUIPMENT,
                                  QStringLiteral("卡片网格展示装备图、名称与获取途径"), equipmentComponents()));
    templates.append(makeTemplate(QStringLiteral("属性/数值对比"), E_WALKTHROUGH_TYPE_STATS_COMPARE,
                                  QStringLiteral("表格为主，对比属性面板与数值"), statsComponents()));
    templates.append(makeTemplate(QStringLiteral("剧情流程"), E_WALKTHROUGH_TYPE_STORY_FLOW,
                                  QStringLiteral("时间线展示章节、关键节点与分支"), storyComponents()));
    templates.append(makeTemplate(QStringLiteral("武器/角色评测"), E_WALKTHROUGH_TYPE_WEAPON_REVIEW,
                                  QStringLiteral("大图 + 评分条 + 优缺点列表"), reviewComponents()));
    templates.append(makeTemplate(QStringLiteral("地图/点位"), E_WALKTHROUGH_TYPE_MAP_POINTS,
                                  QStringLiteral("背景图加标注点"), mapComponents()));
    templates.append(makeTemplate(QStringLiteral("通用封面"), E_WALKTHROUGH_TYPE_COVER,
                                  QStringLiteral("标题、副标题、配图与作者署名"), coverComponents()));
    return templates;
}

} // namespace bwm
