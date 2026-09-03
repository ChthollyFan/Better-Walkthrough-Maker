/**
 * @file Article.cpp
 * @author zhangweimu
 * @brief 文章攻略辅助函数实现。
 */
#include "core/Article.h"

#include <QRegularExpression>

namespace bwm {

// 模块内静态正则对象，避免每次调用重复编译。
// 捕获组 1 = 攻略索引 W，捕获组 2 = 页面索引 P。
static const QRegularExpression kPageRefRegex(kPageRefPattern);

QString makePageRef(int nWalkthroughIndex, int nPageIndex)
{
    return QStringLiteral("![[%1:%2]]").arg(nWalkthroughIndex).arg(nPageIndex);
}

bool containsPageRef(const QString& rText)
{
    return kPageRefRegex.match(rText).hasMatch();
}

} // namespace bwm
