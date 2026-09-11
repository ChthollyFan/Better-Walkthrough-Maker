/**
 * @file AuthorMarkStyle.h
 * @author zhangweimu
 * @brief 署名水印样式数据模型（位置 / 字体 / 字号 / 加粗 / 颜色 / 不透明度）。
 *
 * 样式由「导出对话框 → 署名设置…」修改，经 QSettings 持久化（见 settings/Settings.h），
 * 导出时随 PluginContext 传给各导出 Provider（页面 PNG、文章 PNG、PDF 署名行）。
 *
 * 默认值刻意与历史硬编码行为保持一致：右下角、微软雅黑 18px、不加粗、黑色 63% 不透明
 * （旧实现为 alpha 160）。这样未改过设置的用户，导出结果与旧版本完全相同。
 */
#ifndef BWM_CORE_AUTHORMARKSTYLE_H
#define BWM_CORE_AUTHORMARKSTYLE_H

#include <QColor>
#include <QString>

namespace bwm {

// 署名水印在导出图片中的位置（四角）
enum E_AUTHOR_MARK_POSITION {
    E_AUTHOR_MARK_POSITION_TOP_LEFT = 0,   // 左上角
    E_AUTHOR_MARK_POSITION_TOP_RIGHT,      // 右上角
    E_AUTHOR_MARK_POSITION_BOTTOM_LEFT,    // 左下角
    E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT,   // 右下角（默认，与旧版本一致）
};

/**
 * @brief 署名水印样式。
 *
 * 字号为「逻辑像素」：1x 导出时的实际像素大小，导出时按倍率缩放
 * （页面导出＝导出倍率，文章长图＝图片宽度/720）。PDF 署名行按默认字号比例换算。
 */
struct AuthorMarkStyle {
    // 默认字号（逻辑像素）；PDF 署名行与正文的比例关系由它换算
    static constexpr int nDefaultFontSize = 18;
    // 默认不透明度（%）；63 ≈ 旧版硬编码的 alpha 160
    static constexpr int nDefaultOpacityPercent = 63;

    E_AUTHOR_MARK_POSITION ePosition = E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT;  // 位置
    QString strFontFamily;                 // 字体族；为空表示使用默认字体
    int nFontSize = nDefaultFontSize;      // 字号（逻辑像素，导出时按倍率缩放）
    bool bBold = false;                    // 是否加粗
    QColor color = QColor(0, 0, 0);        // 颜色（仅 RGB，透明度见 nOpacityPercent）
    int nOpacityPercent = nDefaultOpacityPercent;   // 不透明度（0~100）

    // 默认字体族（历史硬编码值）
    static QString defaultFontFamily();
    // 实际使用的字体族：strFontFamily 为空时回退默认字体
    QString resolvedFontFamily() const;
    // 把字号与不透明度收敛到合法区间（读设置时防御脏数据）
    void clamp();
};

// 位置与字符串互转（QSettings 持久化用，字符串形式保证可读与迁移友好）
QString authorMarkPositionToString(E_AUTHOR_MARK_POSITION ePosition);
E_AUTHOR_MARK_POSITION authorMarkPositionFromString(const QString& strPosition);
// 位置的中文名称（设置界面下拉框显示用）
QString authorMarkPositionDisplayName(E_AUTHOR_MARK_POSITION ePosition);

// 相等比较（设置变更检测与单元测试用）；颜色只比较 RGB，透明度单独比较
inline bool operator==(const AuthorMarkStyle& rLeft, const AuthorMarkStyle& rRight)
{
    return rLeft.ePosition == rRight.ePosition
        && rLeft.strFontFamily == rRight.strFontFamily
        && rLeft.nFontSize == rRight.nFontSize
        && rLeft.bBold == rRight.bBold
        && rLeft.color.rgb() == rRight.color.rgb()
        && rLeft.nOpacityPercent == rRight.nOpacityPercent;
}

inline bool operator!=(const AuthorMarkStyle& rLeft, const AuthorMarkStyle& rRight)
{
    return !(rLeft == rRight);
}

} // namespace bwm

#endif // BWM_CORE_AUTHORMARKSTYLE_H
