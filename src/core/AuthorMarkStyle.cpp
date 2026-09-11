/**
 * @file AuthorMarkStyle.cpp
 * @author zhangweimu
 * @brief 署名水印样式实现（默认值、区间收敛、位置字符串互转）。
 */
#include "core/AuthorMarkStyle.h"

#include <QtGlobal>

namespace bwm {

namespace {

// 默认字体族（历史硬编码值，保持中文界面下最稳妥的显示效果）
const QString kDefaultFontFamily = QStringLiteral("Microsoft YaHei");
// 字号区间：与「编辑文本」对话框保持一致，避免两处设置范围不同
constexpr int nMinFontSize = 6;
constexpr int nMaxFontSize = 400;

// 位置字符串常量（持久化键值；不用中文，便于手改配置文件）
const QString kPositionTopLeft = QStringLiteral("top-left");
const QString kPositionTopRight = QStringLiteral("top-right");
const QString kPositionBottomLeft = QStringLiteral("bottom-left");
const QString kPositionBottomRight = QStringLiteral("bottom-right");

} // namespace

QString AuthorMarkStyle::defaultFontFamily()
{
    return kDefaultFontFamily;
}

QString AuthorMarkStyle::resolvedFontFamily() const
{
    const QString strFamily = strFontFamily.trimmed();
    return strFamily.isEmpty() ? kDefaultFontFamily : strFamily;
}

void AuthorMarkStyle::clamp()
{
    nFontSize = qBound(nMinFontSize, nFontSize, nMaxFontSize);
    nOpacityPercent = qBound(0, nOpacityPercent, 100);
}

QString authorMarkPositionToString(E_AUTHOR_MARK_POSITION ePosition)
{
    switch (ePosition) {
    case E_AUTHOR_MARK_POSITION_TOP_LEFT:
        return kPositionTopLeft;
    case E_AUTHOR_MARK_POSITION_TOP_RIGHT:
        return kPositionTopRight;
    case E_AUTHOR_MARK_POSITION_BOTTOM_LEFT:
        return kPositionBottomLeft;
    case E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT:
    default:
        return kPositionBottomRight;
    }
}

E_AUTHOR_MARK_POSITION authorMarkPositionFromString(const QString& strPosition)
{
    // 未知值统一回退默认（右下角），保证手改配置/旧配置不会画出错位水印
    if (strPosition == kPositionTopLeft) {
        return E_AUTHOR_MARK_POSITION_TOP_LEFT;
    }
    if (strPosition == kPositionTopRight) {
        return E_AUTHOR_MARK_POSITION_TOP_RIGHT;
    }
    if (strPosition == kPositionBottomLeft) {
        return E_AUTHOR_MARK_POSITION_BOTTOM_LEFT;
    }
    return E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT;
}

QString authorMarkPositionDisplayName(E_AUTHOR_MARK_POSITION ePosition)
{
    switch (ePosition) {
    case E_AUTHOR_MARK_POSITION_TOP_LEFT:
        return QStringLiteral("左上角");
    case E_AUTHOR_MARK_POSITION_TOP_RIGHT:
        return QStringLiteral("右上角");
    case E_AUTHOR_MARK_POSITION_BOTTOM_LEFT:
        return QStringLiteral("左下角");
    case E_AUTHOR_MARK_POSITION_BOTTOM_RIGHT:
    default:
        return QStringLiteral("右下角");
    }
}

} // namespace bwm
