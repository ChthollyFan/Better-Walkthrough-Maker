/**
 * @file AssetStore.h
 * @author zhangweimu
 * @brief 素材存储工具：外部图片导入项目 assets/ 与项目内图片路径解析。
 *
 * 集中放置「把外部图片复制进项目 assets/」与「把页面/组件里记录的图片路径
 * 解析为可加载的绝对路径」两类逻辑，供图片组件 Provider、页面背景图、
 * 素材面板删除保护等多处复用，避免各写一份。
 */
#ifndef BWM_PROJECT_ASSETSTORE_H
#define BWM_PROJECT_ASSETSTORE_H

#include <QString>

namespace bwm {

/**
 * @brief 项目素材存储工具（静态方法集合）。
 */
class AssetStore
{
public:
    /**
     * @brief 把外部图片复制进项目的 assets/ 目录，用 UUID 命名避免重名。
     *
     * 源文件已在项目 assets/ 内时不再复制，直接返回原路径。
     * 未打开项目（strProjectDir 为空）时不复制，直接返回源路径（与旧行为一致）。
     *
     * @param strSourcePath  源图片路径
     * @param strProjectDir  项目目录（assets/ 的父目录）；空表示无项目
     * @param pErrorMessage  失败原因（可选）
     * @return               导入后的绝对路径；失败返回空字符串
     */
    static QString importImage(const QString& strSourcePath, const QString& strProjectDir,
                               QString* pErrorMessage = nullptr);

    /**
     * @brief 绝对路径 → 项目内相对路径（如 "assets/xxx.png"）。
     *
     * 路径不在项目目录内、或项目目录为空时返回原路径（此时背景图退化为绝对路径，
     * 仍可正常显示，只是项目移动后失效）。
     *
     * @param strAbsolutePath 文件绝对路径
     * @param strProjectDir   项目目录
     * @return                项目内相对路径（分隔符统一为 '/'）
     */
    static QString toProjectRelative(const QString& strAbsolutePath, const QString& strProjectDir);

    /**
     * @brief 把记录的图片路径解析为可加载的绝对路径。
     *
     * - 空路径返回空字符串；
     * - 绝对路径原样返回（兼容旧数据中的绝对路径）；
     * - 相对路径按 strProjectDir 解析；无项目目录时返回原值，由调用方加载失败兜底。
     *
     * @param strPath        页面/组件中记录的图片路径
     * @param strProjectDir  项目目录
     * @return               绝对路径（无法解析时返回原值）
     */
    static QString resolvePath(const QString& strPath, const QString& strProjectDir);
};

} // namespace bwm

#endif // BWM_PROJECT_ASSETSTORE_H
