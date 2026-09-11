# Better-Walkthrough-Maker

**更好的攻略制作器** —— 面向游戏攻略作者的桌面设计工具：用"模板 + 自由画布"的方式制作攻略内容（装备推荐、属性对比、剧情流程等图文配图，以及 Markdown 图文文章），导出 PNG / PDF / Markdown 等格式。

## 功能特性

- **项目管理**：项目 → 攻略 → 页面 三层结构；每页独立画布尺寸；自动保存 + 崩溃恢复；最近项目
- **画布编辑器**：图片/文本/表格/形状/贴纸 五类组件；拖拽/缩放/旋转、多选联动、图层管理、对齐与等距分布、网格与边缘吸附、撤销重做、跨页面复制
- **素材库**：批量导入图片（复制进项目自包含），缩略图预览，双击插入画布
- **模板系统**：六类内置模板（装备推荐/属性对比/剧情流程/武器评测/地图点位/通用封面）+ 空白模板；用户模板保存/导入/导出（.json）
- **美化包**：贴纸装饰（标题装饰线/角标/星标/箭头/分割线/卡片边框，卡片边框支持形状、颜色与框内图片）+ 主题（画布配色管理页面与导出背景色，界面外观可切深色/浅色/跟随系统）
- **导出**：页面支持单页 / 批量 / 长图 PNG（1x/2x/3x），文章支持 Markdown / PNG 长图 / PDF；可选作者署名，一键打开导出目录；复制当前页到剪贴板
- **文章攻略**：Markdown 分栏编辑（源码 + 实时预览），正文可用 `![[W:P]]` 引用攻略页面，支持导入 `.md` 文件

## 使用教程（Wiki）

完整文档见 **[Wiki 首页](wiki/Home.md)**（同步发布在 [GitHub Wiki](https://github.com/ChthollyFan/Better-Walkthrough-Maker/wiki)）。

| 主题 | 页面 |
|---|---|
| 新手入门 | [快速开始](wiki/快速开始.md) — 下载 → 新建项目 → 插入组件 → 导出 PNG |
| 使用手册 | [界面总览](wiki/界面总览.md) · [项目管理](wiki/项目管理.md) · [画布与组件](wiki/画布与组件.md) · [素材库](wiki/素材库.md) |
| 功能专题 | [模板与美化](wiki/模板与美化.md) · [导出](wiki/导出.md) · [文章攻略](wiki/文章攻略.md) · [设置与快捷键](wiki/设置与快捷键.md) |
| 排错 | [常见问题](wiki/常见问题.md) — 含已知问题与尚未实现的功能清单 |
| 开发者 | [开发环境与构建](wiki/开发环境与构建.md) · [项目架构](wiki/项目架构.md) · [数据格式](wiki/数据格式.md) · [插件开发](wiki/插件开发.md) · [打包与发布](wiki/打包与发布.md) · [贡献指南](wiki/贡献指南.md) |

## 下载与发布

[![GitHub Release](https://img.shields.io/github/v/release/ChthollyFan/Better-Walkthrough-Maker)](https://github.com/ChthollyFan/Better-Walkthrough-Maker/releases)

发布产物只有一个：**单文件 `BWM.exe`**（免安装、无需 Qt DLL）。

- **下载使用**：从 [Releases](https://github.com/ChthollyFan/Better-Walkthrough-Maker/releases) 下载 `BWM.exe`，双击即可运行
- **本地打包（单文件，发布用）**：先 `pwsh -File package.ps1` 生成 `release/` 目录，再 `pwsh -File package_single.ps1` 打包出根目录下的 `BWM.exe`（打包工具 `tools/enigmavbconsole.exe` 已随仓库提供）
- **本地打包（文件夹版）**：`pwsh -File package.ps1`（生成 `release/` 目录 + zip 压缩包）；或 `powershell -ExecutionPolicy Bypass -File .\deploy.ps1`（生成 `dist/` 下绿色版 zip）
- **自动发布**：打标签 `git tag v0.3.0 && git push origin v0.3.0`，GitHub Actions 自动构建（装 Qt → 编译 → 测试 → 打包）并发布 `BWM.exe` 到 Releases

## 设计文档

使用说明与开发文档都在 [Wiki](wiki/Home.md)；以下是仓库内的设计记录与规范：

- [项目规划](docs/project-plan.md) — 产品定位、数据模型、功能模块、技术架构、里程碑与实现记录
- [文章攻略设计](docs/article-walkthrough-design.md) — Markdown 文章模块的设计、实现要点与问题复盘
- [C++ 编码规范](docs/c++编码规范.md) — 代码风格与命名约定（开发前请阅读）
- 插件开发流程：[组件](docs/plugin-component-dev.md) · [导出格式](docs/plugin-export-dev.md) · [面板](docs/plugin-panel-dev.md) · [模板包](docs/plugin-template-dev.md) · [主题](docs/plugin-theme-dev.md)
- [Tag 发布流程](AGENTS.md#tag-发布流程) — 版本号、CHANGELOG、打包与发布检查清单

## 许可证

[MIT](LICENSE) © 2026 ChthollyFan

## 构建与测试

依赖：CMake ≥ 3.21、Ninja、MinGW-w64 g++（14.x）、Qt 6（MinGW 版，如 6.11.2）。

```powershell
# 配置（显式指定 MinGW 版 Qt 路径；若环境变量 Qt6_DIR 指向其他版本需覆盖）
cmake -S . -B build -G Ninja "-DCMAKE_PREFIX_PATH=C:/Users/ThinkPad/Qt/6.11.2/mingw_64" "-DQt6_DIR=C:/Users/ThinkPad/Qt/6.11.2/mingw_64/lib/cmake/Qt6" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++

# 编译
cmake --build build

# 运行测试（16 个测试程序）
$env:PATH = "C:/Users/ThinkPad/Qt/6.11.2/mingw_64/bin;" + $env:PATH
ctest --test-dir build --output-on-failure

# 运行主程序（首次构建后需 windeployqt 部署运行时 DLL 到 exe 目录）
windeployqt --release --compiler-runtime build\src\bwm.exe
build\src\bwm.exe
```

## 当前状态

- ✅ 规划定稿（`docs/project-plan.md`）
- ✅ **M1–M7 全部完成**（骨架 / 画布 / 表格与素材库 / 导出 / 模板与美化包 / 打磨 / 文章攻略）
- ✅ 单元测试 16 个测试程序全部通过（序列化 / 项目管理 / 导出渲染 / 模板 / 文章 / 画布层序 / 对话框 / 样式等）
- ✅ 文章攻略：Markdown 分栏编辑、`![[W:P]]` 页面引用、导入导出（Markdown / PNG / PDF）、作者署名
- ✅ 应用图标、导出目录记忆、单文件打包（Enigma Virtual Box）
- ⏳ 后续：目标平台的图片规格实测、安装包发布、GIF 导出、蒙版/滤镜、模板占位符
