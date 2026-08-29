# 导出格式插件开发流程

> 对应接口：`IExportProvider`（`src/plugin/IExportProvider.h`）
>
> 当用户要求开发新的导出格式（如 PDF、SVG、上传到平台 API 等）时，参照本文档并与用户讨论细节。

## 1. 概述

导出格式插件用于扩展攻略的输出方式。实现 `IExportProvider` 接口并注册到 `PluginHost` 后，新格式会自动出现在导出对话框的格式下拉框中。用户选择格式并确认后，框架调用 `exportPages` 执行导出。

当前内置实现：PNG 逐页导出、PNG 长图导出（见 `src/plugin/builtin/BuiltinExportProviders.cpp`）。

## 2. 接口签名

```cpp
class IExportProvider
{
public:
    virtual ~IExportProvider() = default;

    // 导出格式唯一标识（如 "png.separate"、"pdf"）
    virtual QString formatId() const = 0;

    // 用户可见的格式名称（如"逐页 PNG"、"PDF 文档"）
    virtual QString displayName() const = 0;

    // 执行导出
    virtual int exportPages(const QVector<Page>& vecPages,
                            const QString& rWalkthroughTitle,
                            const QString& strDirPath,
                            qreal dScale,
                            const QString& strAuthor,
                            const PluginContext& rContext,
                            QWidget* pParent) const = 0;
};
```

### exportPages 参数说明

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `vecPages` | `const QVector<Page>&` | 要导出的页面列表 |
| `rWalkthroughTitle` | `const QString&` | 当前攻略标题（用于生成文件名） |
| `strDirPath` | `const QString&` | 导出目标目录 |
| `dScale` | `qreal` | 分辨率倍率（1.0 = 原尺寸，2.0 = 两倍） |
| `strAuthor` | `const QString&` | 作者署名（空字符串表示不署名） |
| `rContext` | `const PluginContext&` | 插件上下文（提供主题背景色等） |
| `pParent` | `QWidget*` | 父窗口（用于显示进度对话框等） |
| **返回值** | `int` | 导出成功的文件数量（0 表示失败） |

## 3. 实现步骤

### 步骤一：确定导出方案

> **讨论点**：与用户确认以下细节：
> - 导出目标格式（PDF / SVG / 图片序列 / JSON / 平台 API）
> - 是否需要额外依赖库（如 Qt PDF 模块、第三方库）
> - 文件命名规则
> - 是否需要进度对话框
> - 是否需要作者署名水印

### 步骤二：创建 Provider 头文件

```cpp
// src/plugin/builtin/PdfExportProvider.h
#ifndef BWM_PLUGIN_BUILTIN_PDFEXPORTPROVIDER_H
#define BWM_PLUGIN_BUILTIN_PDFEXPORTPROVIDER_H

#include "plugin/IExportProvider.h"

namespace bwm {

// 示例：PDF 导出 Provider
class PdfExportProvider : public IExportProvider
{
public:
    QString formatId() const override;
    QString displayName() const override;
    int exportPages(const QVector<Page>& vecPages,
                    const QString& rWalkthroughTitle,
                    const QString& strDirPath,
                    qreal dScale,
                    const QString& strAuthor,
                    const PluginContext& rContext,
                    QWidget* pParent) const override;
};

} // namespace bwm

#endif
```

### 步骤三：实现 Provider

```cpp
// src/plugin/builtin/PdfExportProvider.cpp
#include "plugin/builtin/PdfExportProvider.h"

#include "export/ExportRenderer.h"  // 复用现有渲染器

#include <QDir>
#include <QImage>
#include <QMessageBox>
#include <QProgressDialog>
#include <QRegularExpression>

namespace bwm {

// 匿名命名空间：文件名净化辅助
namespace {
QString sanitizeFileName(const QString& rTitle)
{
    QString strSafe = rTitle;
    strSafe.replace(QRegularExpression(QStringLiteral(R"([\\/:*?\"<>|])")), QStringLiteral("_"));
    if(strSafe.trimmed().isEmpty()) {
        strSafe = QStringLiteral("攻略");
    }
    return strSafe;
}
} // namespace

QString PdfExportProvider::formatId() const
{
    return QStringLiteral("pdf");
}

QString PdfExportProvider::displayName() const
{
    return QStringLiteral("PDF 文档");
}

int PdfExportProvider::exportPages(const QVector<Page>& vecPages,
                                   const QString& rWalkthroughTitle,
                                   const QString& strDirPath,
                                   qreal dScale,
                                   const QString& strAuthor,
                                   const PluginContext& rContext,
                                   QWidget* pParent) const
{
    const QString strSafeTitle = sanitizeFileName(rWalkthroughTitle);
    const QString strCleanDir = QDir::cleanPath(strDirPath);
    const QString strFilePath = QDir(strCleanDir).filePath(strSafeTitle + QStringLiteral(".pdf"));

    // TODO: 实现实际的 PDF 导出逻辑
    // 可用 ExportRenderer::renderPage 渲染每页为 QImage，再嵌入 PDF
    // 或使用 Qt PDF 模块（QtPdf）直接绘制

    QProgressDialog progress(QStringLiteral("正在导出 PDF…"), QString(), 0, vecPages.size(), pParent);
    progress.setWindowModality(Qt::WindowModal);
    int nExported = 0;
    for(int nIndex = 0; nIndex < vecPages.size(); ++nIndex) {
        progress.setValue(nIndex);
        if(progress.wasCanceled()) {
            break;
        }
        // TODO: 渲染页面并写入 PDF
        ++nExported;
    }
    progress.setValue(vecPages.size());

    if(nExported > 0) {
        QMessageBox::information(pParent, QStringLiteral("导出完成"),
                                 QStringLiteral("已导出到：%1").arg(strFilePath));
    }
    return nExported;
}

} // namespace bwm
```

### 步骤四：注册到 PluginHost

在 `src/plugin/builtin/BuiltinPluginRegistrar.cpp` 中添加：

```cpp
#include "plugin/builtin/PdfExportProvider.h"

// 在 registerBuiltinPlugins 函数体内添加：
static PdfExportProvider* s_pPdf = new PdfExportProvider;
pHost->registerExportProvider(s_pPdf);
```

### 步骤五：更新 CMakeLists.txt

在 `src/CMakeLists.txt` 的 `bwm_core` 目标源文件列表中添加：

```cmake
plugin/builtin/PdfExportProvider.cpp
```

如果引入了第三方依赖（如 Qt PDF 模块），还需在 CMake 中添加 `find_package` 和 `target_link_libraries`。

## 4. 验证清单

- [ ] 编译通过
- [ ] 单元测试通过
- [ ] 导出对话框格式下拉框中出现新格式
- [ ] 选择新格式后导出能正常执行
- [ ] 导出的文件可在目标程序中正常打开
- [ ] 文件名净化正确（特殊字符替换为下划线）
- [ ] 进度对话框正常显示且可取消
- [ ] 导出完成后有成功提示

## 5. 注意事项

- `ExportRenderer::renderPage` 和 `ExportRenderer::renderLongImage` 可复用，它们把页面渲染为 `QImage`，导出插件可在此基础上转换为其他格式。
- 文件名净化是各 Provider 的责任（参考示例中的 `sanitizeFileName` 辅助函数）。
- 导出是同步操作，长时间导出会阻塞 UI。如需异步导出，应在 Provider 内部使用 `QThread` 或 `QtConcurrent`。
- 当前导出对话框对"当前页"范围会禁用格式选择（因为单页导出格式无差异）。如果新格式在单页时有特殊行为，需要修改 `ExportDialog` 逻辑。

## 6. 未来升级路径

当前为编译时静态注册。未来实现动态库加载后，导出格式插件可编译为独立 `.dll`/`.so`，在初始化时调用 `PluginHost::registerExportProvider` 注册。
