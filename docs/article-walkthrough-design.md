# 文章攻略（Markdown）模块设计方案

> 状态：设计已确认，待分期实施
> 日期：2025-01

## 一、产品定位

在现有"图文攻略（Walkthrough，画布 + 组件 → PNG）"之外，新增平级的**文章攻略（Article）**：基于 Markdown 的富文本攻略，支持嵌入图片、引用项目内图文攻略页面，可编辑 / 预览 / 导入 / 导出。

用户右键项目树时可选择"新建页面"（图文）或"新建文章"（Markdown）。

## 二、技术前提（已验证）

| 能力 | Qt 提供 | 是否需新增依赖 |
|---|---|---|
| Markdown 渲染 | `QTextDocument::setMarkdown()`，支持 CommonMark + GFM | 否，Widgets 自带 |
| 图片嵌入 | 重写 `QTextDocument::loadResource()` 注入项目内图片 | 否 |
| PDF 导出 | `QPrinter(PdfFormat)` + `QTextDocument::print()` 多页分页 | 否 |
| PNG 长图导出 | `QTextDocument::drawContents()` + `QPainter` → QImage | 否 |

**结论：无需新增 Qt 模块依赖，现有 `find_package(Qt6 REQUIRED COMPONENTS Widgets)` 足够。**

## 三、数据模型变更（src/core/）

### 3.1 新增 Article 结构

```cpp
// 文章攻略：基于 Markdown 的文本攻略，与 Walkthrough（图文攻略）平级。
struct Article {
    QString strId;           // 唯一 id
    QString strTitle;        // 文章标题
    QString strMarkdown;     // Markdown 正文（内嵌，含 ![[W:P]] 页面引用与 ![](assets/...) 图片引用）
};
```

### 3.2 Project 新增字段

```cpp
struct Project {
    // ... 现有字段不变 ...
    QVector<Article> vecArticles;   // 文章攻略列表（与 vecWalkthroughs 平级）
};
```

### 3.3 序列化（ProjectSerializer）

- `walkthroughs` 之外新增 `articles` 数组，每个元素 `{id, title, markdown}`。
- `formatVersion` 保持 1 不变（新增字段对旧文件向后兼容：缺失时 `vecArticles` 为空）。
- Markdown 正文内嵌 project.json。几万字以内完全无压力，`QJsonDocument` 处理几百 KB JSON 很轻松。
- 旧版本软件打开含 articles 的新文件时，未知字段被 QJsonDocument 容错忽略，不会崩溃。

### 3.4 页面引用语法（本项目自定义扩展）

- 语法：`![[W:P]]` 表示引用第 W 个图文攻略的第 P 页（类似 Obsidian 嵌入语法，与标准图片 `![](url)` 视觉区分）。
- 解析：正则 `!\[\[(\d+):(\d+)\]\]` 匹配，用现有 `ExportRenderer::renderPage()` 渲染该页为 QImage，通过 `QTextDocument::addResource(ImageResource, ...)` 注入显示。
- 该语法不干扰标准 Markdown 解析（`setMarkdown` 会把它当普通文本，我们在渲染前后做替换注入）。

## 四、UI 变更（src/app/）

### 4.1 中央区域切换

MainWindow 中央区域由"纯画布"改为**按当前选中节点类型切换**（QStackedWidget）：

- 选中页面节点（图文）→ CanvasView（现有行为不变）
- 选中文章节点 → ArticleEditor（新）

### 4.2 ArticleEditor（新增 src/app/panels/ArticleEditor.h/.cpp）

分栏布局（QSplitter），对标 Typora / 小黑盒编辑器：

- **左侧**：`QTextEdit` 纯文本编辑（Markdown 源码）。工具栏按钮：加粗 / 斜体 / 标题 / 列表 / 链接 / 图片 / 页面引用。
- **右侧**：`QTextBrowser` 实时预览。`setMarkdown()` 渲染，重写 `loadResource` 加载项目内图片与页面引用渲染图。
- 左侧编辑防抖（QTimer 300ms）触发右侧重新预览。

### 4.3 项目树面板变更（ProjectTreePanel）

**组织方式：扁平混排靠图标区分**（已确认）

- 所有攻略平级列出，图文攻略与文章攻略用不同图标区分。
- 节点键格式扩展：
  - `""` → 项目根
  - `"W"` → 图文攻略节点
  - `"W:P"` → 页面节点
  - `"A"` → 文章攻略节点（新增）
- 右键项目根：新建图文攻略 / 新建文章（含"从 .md 文件导入"）。
- 右键文章节点：重命名、删除、导出。
- 选中文章节点 → 发出 `articleSelected(QString articleKey)` 信号 → MainWindow 切换到 ArticleEditor。

### 4.4 导出对话框扩展（ExportDialog）

当当前对象是文章时，格式下拉框出现：

- **Markdown (.md)**：导出原始 .md + images/ 目录（页面引用渲染图）+ 兼容版 .md
- **PNG 长图**：渲染文章为单张长图 PNG
- **PDF**：渲染文章为分页 PDF

## 五、插件接口变更（src/plugin/）

`IExportProvider` 现有 `exportPages(QVector<Page>)` 只针对页面。为支持文章导出，**增加重载方法（向后兼容）**：

```cpp
class IExportProvider {
    // 现有：页面型导出（图文攻略），不变
    virtual int exportPages(const QVector<Page>& vecPages, ...) = 0;
    // 新增：文章型导出，默认实现返回 0（不支持）
    virtual int exportArticle(const Article& rArticle, const Project& rProject,
                              const QString& strDirPath, const PluginContext& rContext,
                              QWidget* pParent) const { return 0; }
    // 新增：声明是否支持文章导出，导出对话框据此过滤
    virtual bool supportsArticle() const { return false; }
};
```

ExportDialog 根据当前选中对象类型（页面 vs 文章）+ Provider 的 `supportsArticle()` 过滤可选格式。

## 六、导入（.md → Article）

- 项目树右键"新建文章 → 从文件导入"，读取 .md 存入 `Article::strMarkdown`。
- .md 中本地图片 `![](xxx.png)`：若图片在同目录，复制进项目 `assets/` 并改写路径为 `![](assets/xxx.png)`。
- 外部 .md 中的 `![[W:P]]` 原样保留（视为普通文本，不影响导入）。

## 七、导出（Article → .md + PNG + PDF）

| 格式 | 处理 |
|---|---|
| .md | 主文件原样输出（保留 `![[W:P]]`）；images/ 导出页面引用渲染图；额外输出兼容版 .md（`![[W:P]]` → `![](images/page_W_P.png)`） |
| PNG 长图 | 构造 QTextDocument 渲染，QPainter 画到单张高 QImage（宽 1080，高自适应） |
| PDF | 同上构造 QTextDocument，QPrinter(PdfFormat) + print() 自动分页 |

### 7.1 导出 .md 细节

- 主文件：原样输出 `Article::strMarkdown`（保留 `![[W:P]]`）。
- images/ 目录：对所有 `![[W:P]]` 引用，用 `ExportRenderer::renderPage()` 渲染为 `images/page_W_P.png`。
- 兼容版 `*_compatible.md`：把 `![[W:P]]` 替换为 `![](images/page_W_P.png)`，外部 Markdown 阅读器可直接看图。
- 对 `![](assets/xxx.png)` 引用：把对应素材文件复制到 images/，路径改写为相对路径。

### 7.2 导出 PNG 长图

- 构造 QTextDocument，setMarkdown + 注入所有图片 / 页面引用资源。
- 用 QPainter 渲染到单张高 QImage（宽度固定如 1080，高度按文档内容自适应）。

### 7.3 导出 PDF

- 同上构造 QTextDocument。
- QPrinter(PdfFormat) + QTextDocument::print()，自动多页分页。

## 八、新增 / 修改文件清单

### 新增文件

| 文件 | 作用 |
|---|---|
| `src/core/Article.h` / `.cpp` | Article 结构与辅助函数 |
| `src/app/panels/ArticleEditor.h` / `.cpp` | 分栏编辑器（左编辑右预览） |
| `src/app/panels/MarkdownPreview.h` / `.cpp` | 预览控件（重写 loadResource，处理页面引用） |
| `src/plugin/builtin/BuiltinArticleExportProviders.h` / `.cpp` | 文章导出 Provider（md / png / pdf） |
| `src/core/ArticlePageRef.h` / `.cpp` | `![[W:P]]` 解析与渲染工具 |
| `tests/core/test_article.cpp` | Article 序列化测试 |
| `tests/core/test_articlepageref.cpp` | 页面引用解析测试 |

### 修改文件

| 文件 | 修改内容 |
|---|---|
| `src/core/Project.h` | 新增 `Article` 结构与 `vecArticles` 字段 |
| `src/core/ProjectSerializer.cpp` | 序列化 / 反序列化 articles 数组 |
| `src/app/panels/ProjectTreePanel.h` / `.cpp` | 文章节点显示、右键菜单、图标区分 |
| `src/app/MainWindow.h` / `.cpp` | 中央 QStackedWidget 切换、articleSelected 信号处理 |
| `src/app/dialogs/ExportDialog.h` / `.cpp` | 文章导出格式选项 |
| `src/plugin/IExportProvider.h` | 新增 exportArticle / supportsArticle |
| `src/plugin/builtin/BuiltinPluginRegistrar.cpp` | 注册文章导出 Provider |
| `src/CMakeLists.txt` | 新增源文件 |
| `docs/project-plan.md` | 补充文章攻略模块说明 |

## 九、实施分期建议

每期可独立编译提交，保持可编译可测试：

### 第一期：数据层

- Article 结构定义
- ProjectSerializer 读写 articles
- 单元测试（序列化往返、向后兼容）
- 可独立编译验证

### 第二期：编辑器

- ArticleEditor 分栏编辑器
- MarkdownPreview 预览控件
- 项目树扩展（文章节点、图标、右键菜单）
- MainWindow 中央切换
- 暂不含页面引用渲染（`![[W:P]]` 显示为原文）

### 第三期：页面引用

- `![[W:P]]` 解析（ArticlePageRef）
- 预览时渲染注入（ExportRenderer::renderPage → QTextDocument resource）
- 编辑器工具栏"插入页面引用"按钮

### 第四期：导入导出

- .md 文件导入（含图片复制与路径改写）
- .md 导出（原样 + 兼容版 + images/）
- PNG 长图导出
- PDF 导出
- ExportDialog 扩展

每期完成后向用户汇报，确认后再进入下一期。
