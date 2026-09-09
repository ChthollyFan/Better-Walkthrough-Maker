# 文章攻略（Markdown）模块设计

> 状态：✅ 已实现（四期全部完成）
> 日期：2025-01 设计定稿 / 实施完成
> 相关提交：`a9c9c8d`（数据层）、`7189c95`（编辑器）、`ad574f4`（攻略内嵌重构）、`27882f5`（页面引用渲染）、`6679194`（导入导出）

## 一、产品定位

在"图文攻略（画布 + 组件 → PNG）"之外，新增基于 Markdown 的**文章攻略**，支持嵌入图片、引用项目内图文攻略页面，可编辑 / 预览 / 导入 / 导出。

**关键设计决定**：文章不是与图文攻略平级的独立概念，而是**攻略（Walkthrough）的子级**——一个攻略下可同时包含页面（图文）和文章（Markdown），两者是同级兄弟。

用户右键项目树中的**攻略节点**即可选择"新建页面"或"新建文章"。

## 二、技术前提

| 能力 | Qt 提供 | 依赖 |
|---|---|---|
| Markdown 渲染 | `QTextDocument::setMarkdown()`，支持 CommonMark + GFM | Qt Widgets（已有） |
| 图片嵌入 | `QTextDocument::addResource(ImageResource, ...)` | Qt Widgets（已有） |
| PDF 导出 | `QPrinter(PdfFormat)` + `QTextDocument::print()` 自动分页 | **Qt PrintSupport（新增）** |
| PNG 长图导出 | `QTextDocument::drawContents()` + `QPainter` → QImage | Qt Widgets（已有） |

**新增依赖**：`Qt6::PrintSupport`（PDF 导出用）。部署时 `windeployqt` 会自动带上 `Qt6PrintSupport.dll`。

## 三、数据模型（src/core/）

### 3.1 Article 结构

```cpp
// 文章攻略：基于 Markdown 的文本攻略。
struct Article {
    QString strId;           // 唯一 id（QUuid 字符串）
    QString strTitle;        // 文章标题
    QString strMarkdown;     // Markdown 正文（内嵌，含 ![[W:P]] 页面引用与 ![](assets/...) 图片引用）
};
```

### 3.2 Walkthrough 内嵌文章

```cpp
struct Walkthrough {
    QString strTitle;
    E_WALKTHROUGH_TYPE eType;
    QVector<Page> vecPages;        // 页面（图文）
    QVector<Article> vecArticles;  // 文章（Markdown，与页面同级）
};
```

### 3.3 序列化（ProjectSerializer）

- 攻略对象内新增 `articles` 数组，每个元素 `{id, title, markdown}`。
- `formatVersion` 保持 1：缺失字段取默认值，旧文件可正常打开。
- **旧格式迁移**：早期版本把 articles 存在 project 级（`Project::vecArticles`，现已废弃）。
  反序列化时自动迁移到第一个攻略内；若项目无攻略则新建"迁移文章"攻略存放。
- Markdown 正文内嵌 `project.json`，几万字以内无性能压力。

### 3.4 页面引用语法（本项目自定义扩展）

- 语法：`![[W:P]]` —— 引用第 W 个攻略的第 P 个页面（类似 Obsidian 嵌入语法，与标准图片 `![](url)` 视觉区分）。
- 解析：正则 `!\[\[(\d+):(\d+)\]\]`（`kPageRefPattern`，见 `core/Article.h`）。
- 渲染：替换为 `![](bwm://page/W/P)` 图片语法 → `setMarkdown` 后注册 `QImage` 为 document resource。
- 标准 Markdown 解析器不会误判（该语法在标准 Markdown 中是普通文本）。

## 四、UI 实现（src/app/）

### 4.1 中央区域切换

`MainWindow` 中央区域是 `QStackedWidget`：

- 选中页面节点 → index 0（`CanvasView` 画布），右侧显示素材库 / 图层面板
- 选中文章节点 → index 1（`ArticleEditor`），**隐藏**右侧面板（它们是画布专用）

### 4.2 ArticleEditor（分栏编辑器）

`src/app/panels/ArticleEditor.h/.cpp`：

- 左侧 `QTextEdit`：Markdown 源码编辑（等宽字体）
- 右侧 `MarkdownPreview`：实时预览，300ms 防抖
- 工具栏：加粗 / 斜体 / 标题 / 列表 / 链接 / 图片 / 页面引用
- 页面引用按钮弹出**图形化页面选择器**（列出所有攻略的所有页面），无需手输 W:P
- 图片按钮：选本地图片 → 复制进项目 `assets/` → 插入相对路径引用
- 编辑后发出 `articleModified` 信号，MainWindow 同步回模型并标记 dirty

### 4.3 MarkdownPreview（预览控件）

`src/app/panels/MarkdownPreview.h/.cpp`：

- 继承 `QTextBrowser`，`setMarkdownSource()` 渲染
- 解析 `![[W:P]]` → 渲染页面为 QImage（0.5 倍率）→ 注册 resource
- 遍历 document 中所有 `QTextImageFormat`，加载普通图片并注册 resource
- 图片最大宽度限制为预览区宽度，避免大图撑满

### 4.4 项目树（ProjectTreePanel）

节点键格式：

| 键 | 含义 |
|---|---|
| `""` | 项目根 |
| `"W"` | 攻略节点 |
| `"W:P"` | 页面节点 |
| `"W:A文章索引"` | 文章节点（如 `"0:A0"`） |

右键菜单：

- 项目根 → 新建攻略
- 攻略节点 → 新建页面 / 新建文章 / **从文件导入文章** / 重命名 / 删除
- 文章节点 → 重命名文章 / 删除文章

## 五、导入导出

### 5.1 导入（.md → Article）

`src/export/ArticleImporter.h/.cpp`：

- 读取 `.md` / `.markdown` / `.txt`，标题取文件名（不含扩展名）
- 本地图片 `![](xxx.png)`：若图片与 .md 同目录，复制到项目 `assets/`（UUID 命名避免重名）并改写路径
- 跳过网络 URL（`http://` / `https://`）和已是 `assets/` 的路径
- 外部 .md 中的 `![[W:P]]` 原样保留

### 5.2 导出 Provider（src/plugin/builtin/BuiltinArticleExportProviders.*）

| 格式 | formatId | 产物 |
|---|---|---|
| Markdown 文件 | `article.markdown` | `文章名.md`（原样，保留 `![[W:P]]`）+ `文章名_compatible.md`（引用替换为图片）+ `images/`（页面渲染图 + 素材副本） |
| PNG 长图 | `article.png.longimage` | `文章名.png`，宽 1080，高度自适应 |
| PDF 文档 | `article.pdf` | `文章名.pdf`，A4 自动分页 |

### 5.3 IExportProvider 接口扩展

为支持文章导出，在原有 `exportPages()` 基础上新增（**向后兼容**）：

```cpp
// 是否支持文章导出（页面型 Provider 返回 false）
virtual bool supportsArticle() const { return false; }

// 文章导出（默认返回 0 = 不支持）
virtual int exportArticle(const Article& rArticle, const Project& rProject,
                           const QString& rArticleTitle, const QString& strDirPath,
                           const PluginContext& rContext, QWidget* pParent) const
{ return 0; }
```

`ExportDialog` 根据当前选中对象（文章 / 页面）自动切换模式：文章模式只列出 `supportsArticle()` 为 true 的 Provider，并隐藏范围 / 倍率选项。

### 5.4 导出完成提示

`src/export/ExportResultHelper.h/.cpp` 提供共享的 `showExportResult()`：
显示"导出完成"对话框 + "打开目录"按钮（页面导出与文章导出统一体验）。

**约定**：`pParent` 为 `nullptr` 时不显示提示——便于自动化测试静默调用（否则模态对话框会阻塞测试）。

## 六、渲染实现要点（src/export/ArticleRenderer.*）

文章渲染辅助工具，供三个导出 Provider 共用：

```cpp
// 构造渲染好的 QTextDocument（调用方负责 delete）
static QTextDocument* buildDocument(const Article& rArticle, const Project& rProject,
                                     const PluginContext& rContext, int nImageWidth = 1000);
```

处理流程：

1. 解析 `![[W:P]]` → 替换为 `![](bwm://page/W/P)`
2. 设置默认样式表：**文字颜色跟随主题**（修复深色主题下黑字不可见）+ **字号按图片宽度缩放**
3. `setMarkdown()` 渲染
4. 注册页面引用图片 resource
5. 遍历图片：加载普通图片、注册 resource、限制最大宽度
6. 遍历文字 fragment：统一设置前景色为主题文字色（跳过图片）

### 字号策略

字号与图片宽度成比例：`正文 = 宽度 × 44 / 1080`

| 元素 | 1080 宽时 | 800 宽时（PDF） |
|---|---|---|
| 一级标题 | 88 px | 65 px |
| 二级标题 | 66 px | 48 px |
| 三级标题 | 53 px | 39 px |
| 正文 | 44 px（一行约 24 字） | 32 px |
| 代码块 | 40 px | 29 px |

## 七、文件清单

### 新增

| 文件 | 作用 |
|---|---|
| `src/core/Article.h/.cpp` | Article 结构 + 页面引用工具函数（`makePageRef` / `containsPageRef`） |
| `src/app/panels/ArticleEditor.h/.cpp` | 分栏编辑器 |
| `src/app/panels/MarkdownPreview.h/.cpp` | 预览控件 |
| `src/export/ArticleRenderer.h/.cpp` | 渲染辅助（页面引用、图片、字号、颜色） |
| `src/export/ArticleImporter.h/.cpp` | .md 导入 |
| `src/export/ExportResultHelper.h/.cpp` | 导出完成提示共享实现 |
| `src/plugin/builtin/BuiltinArticleExportProviders.h/.cpp` | 三个文章导出 Provider |
| `tests/core/test_article.cpp` | 数据模型与序列化测试（9 个用例） |
| `tests/export/test_article_export.cpp` | 导入导出测试（10 个用例） |

### 修改

| 文件 | 改动 |
|---|---|
| `src/core/Project.h` | `Walkthrough` 新增 `vecArticles`；`Project::vecArticles` 标记废弃 |
| `src/core/ProjectSerializer.cpp` | 攻略内 articles 读写 + 旧格式迁移 |
| `src/app/MainWindow.h/.cpp` | 中央 `QStackedWidget` 切换、文章信号路由、导出入口 |
| `src/app/panels/ProjectTreePanel.h/.cpp` | 文章节点、右键菜单、从文件导入 |
| `src/app/dialogs/ExportDialog.h/.cpp` | 文章导出模式 |
| `src/plugin/IExportProvider.h` | `supportsArticle` / `exportArticle` |
| `src/plugin/builtin/BuiltinPluginRegistrar.cpp` | 注册三个文章导出 Provider |
| `src/plugin/builtin/BuiltinExportProviders.cpp` | 改用共享 `showExportResult` |
| `CMakeLists.txt` / `src/CMakeLists.txt` / `tests/CMakeLists.txt` | 新源文件 + PrintSupport 依赖 |

## 八、测试覆盖

```
ctest 6/6 通过

test_article（9 个用例）
  ├─ 攻略内文章序列化往返
  ├─ ![[W:P]] 标记往返保留
  ├─ 旧文件无 articles 字段兼容
  ├─ 旧格式 project 级 articles 自动迁移
  ├─ 页面与文章混合
  ├─ 缺失字段取默认值
  ├─ 损坏条目跳过
  └─ makePageRef / containsPageRef

test_article_export（10 个用例）
  ├─ .md 基本导入
  ├─ 导入时复制图片并改写路径
  ├─ buildDocument 构造文档
  ├─ Markdown 导出（主文件 + 兼容版）
  ├─ PNG 导出
  ├─ 深色主题下文字可见（像素级验证，防回归）
  ├─ PDF 导出
  └─ 文件名净化
```

## 九、实施记录

| 期次 | 提交 | 内容 |
|---|---|---|
| 一 | `a9c9c8d` | Article 结构 + ProjectSerializer + 测试 |
| 二 | `7189c95` | ArticleEditor + MarkdownPreview + 项目树文章节点 |
| 修复 | `ad574f4` | 文章改为攻略内嵌（原设计为平级）+ 选中文章时隐藏右侧面板 |
| 三 | `27882f5` | `![[W:P]]` 预览渲染 + 图形化页面选择器 + 图片渲染修复 |
| 四 | `6679194` | .md 导入 + 三种导出格式 + 完成提示 + 字号调整 |

### 实施中发现并修复的问题

1. **moc 对 raw string 的解析**：`test_article.cpp` 中的多行 raw string 导致 moc 报 "missing ')' in macro usage"，
   改用字符串拼接构造 JSON 测试数据。
2. **图片 resource 注册顺序**：必须在 `setMarkdown()` **之后**注册 resource，否则被重建的 document 清除。
3. **图片 URL 匹配**：`QTextImageFormat::name()` 返回的 URL 与原始 Markdown 一致，
   但必须用 `QTextBlock::iterator` 遍历 fragment 获取（`QTextCursor` 逐字符移动拿不到）。
4. **项目目录未设置**：`ArticleEditor` 构造时项目尚未打开，`setProjectDirectory` 未生效导致图片路径无法解析，
   改为在 `onProjectOpened` 时通过 `setProjectContext()` 更新。
5. **深色主题文字不可见**：`setMarkdown` 默认黑字 + 深色背景 → 设置主题文字色。
6. **字号过小**：默认 12pt 在 1080 宽图上过小 → 按宽度比例缩放。
7. **模态对话框阻塞测试**：`showExportResult` 在测试中无人点击导致 90 秒超时 →
   约定 `pParent == nullptr` 时静默。
8. **链接失败（Permission denied）**：`bwm.exe` 正在运行导致链接器无法写入，编译前需先结束进程。

## 十、后续可扩展方向

- 文章内嵌表格的富样式（当前表格由 Markdown 语法生成，样式固定）
- 页面引用的尺寸控制（如 `![[0:0|600]]` 指定宽度）
- 文章目录（根据标题自动生成 TOC）
- 导出时选择「浅色 / 深色」主题（当前跟随项目主题）
