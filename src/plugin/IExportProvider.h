/**
 * @file IExportProvider.h
 * @author zhangweimu
 * @brief 导出格式插件接口：第三方可实现此接口注册新的导出格式。
 *
 * 实现此接口并注册到 PluginHost 后，对应格式会自动出现在导出对话框的
 * 格式选择中。用户选择格式并确认后，框架调用 exportPages 执行导出。
 *
 * 当前内置实现：PNG 逐页导出、PNG 长图导出（见 BuiltinExportProviders）。
 * 未来可扩展 PDF、SVG、上传到平台 API 等。
 */
#ifndef BWM_PLUGIN_IEXPORTPROVIDER_H
#define BWM_PLUGIN_IEXPORTPROVIDER_H

#include <QMetaType>
#include <QString>
#include <QVector>

#include "core/Project.h"
#include "plugin/PluginContext.h"

class QWidget;   // 前向声明：exportPages 参数用

namespace bwm {

/**
 * @brief 导出格式插件接口。
 *
 * 每个实现代表一种导出方式（如逐页 PNG、长图 PNG、PDF 等）。
 */
class IExportProvider
{
public:
    virtual ~IExportProvider() = default;

    /**
     * @brief 导出格式唯一标识（如 "png.separate"、"png.longimage"）。
     */
    virtual QString formatId() const = 0;

    /**
     * @brief 用户可见的格式名称（如"逐页 PNG"、"长图（纵向拼接）"）。
     * 用于导出对话框的格式下拉框。
     */
    virtual QString displayName() const = 0;

    /**
     * @brief 执行导出。
     *
     * @param vecPages       要导出的页面列表
     * @param rWalkthroughTitle  当前攻略标题（用于生成文件名）
     * @param strDirPath     导出目标目录
     * @param dScale         分辨率倍率（1.0 = 原尺寸，2.0 = 两倍）
     * @param strAuthor      作者署名（空字符串表示不署名）
     * @param rContext       插件上下文（提供主题背景色等）
     * @param pParent        父窗口（用于显示进度对话框等）
     * @return               导出成功后的文件数量（0 表示失败）
     *
     * 实现方负责文件名净化、进度显示、文件写入等全部细节。
     */
    virtual int exportPages(const QVector<Page>& vecPages,
                            const QString& rWalkthroughTitle,
                            const QString& strDirPath,
                            qreal dScale,
                            const QString& strAuthor,
                            const PluginContext& rContext,
                            QWidget* pParent) const = 0;
};

} // namespace bwm

// 注册 IExportProvider* 为 Qt 元类型，使其可存入 QVariant。
// ExportDialog 用 QVariant::fromValue 将 Provider 指针存入 QComboBox 的 itemData。
Q_DECLARE_METATYPE(bwm::IExportProvider*)

#endif // BWM_PLUGIN_IEXPORTPROVIDER_H
