# Repository Guidelines

## 项目结构与模块组织

当前结构（M1 骨架已落地）：

- `src/app/` — 入口与主窗口。
- `src/core/` — 数据模型与 `project.json` 序列化。
- `src/project/` — 项目管理（新建/打开/自动保存/崩溃恢复）。
- `src/settings/` — 全局设置（QSettings）。
- `tests/` — 单元测试（Qt Test），目录结构镜像 `src/`。
- `docs/` — 项目规划等文档。

约定：源码放在 `src/` 按功能模块组织；测试放在 `tests/` 镜像 `src/`；其他文档放在 `docs/`。核心库目标为 `bwm_core`（静态库），主程序与测试共用。

## 构建、测试与开发命令

本机开发环境：Windows + CMake + Ninja + MinGW-w64 g++ 14.2 + Qt 6.11.2（MinGW 版，位于 `C:\Users\ThinkPad\Qt\6.11.2\mingw_64`，由 aqtinstall 的 7z 包手动解压安装；`D:\software\Qt` 另有 MSVC 版 Qt 6.11.1，其 `Qt6_DIR` 环境变量会干扰 CMake 探测，配置时需覆盖）。

```powershell
# 配置（显式指定 MinGW 版 Qt，覆盖环境变量 Qt6_DIR 的干扰）
cmake -S . -B build -G Ninja "-DCMAKE_PREFIX_PATH=C:/Users/ThinkPad/Qt/6.11.2/mingw_64" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++

# 编译
cmake --build build

# 运行测试（Qt 与 MinGW 运行时 DLL 在 Qt 的 bin 目录）
$env:PATH = "C:/Users/ThinkPad/Qt/6.11.2/mingw_64/bin;" + $env:PATH
ctest --test-dir build --output-on-failure

# 运行主程序
build\src\bwm.exe
```

注意：本会话沙箱环境下，CMake 与编译命令需要全权限（其内部启动 g++ 并捕获输出，受沙箱管道限制会挂起）。

## 编码风格与命名规范

目前尚无代码，也未强制任何风格规则。在加入格式化或静态检查工具之前：

- 遵循语言社区的标准格式，保持提交差异精简。
- 文件、类型、函数和变量命名应描述性强、避免缩写。
- 不提交生成物或环境相关文件；出现构建产物后及时补充 `.gitignore`。

## 测试指南

目前尚无测试。新增测试时应：

- 将测试放在 `tests/`，与所测代码对应，一个测试覆盖一个行为。
- 命名采用 `test_<单元>_<行为>` 的形式（例如 `test_parser_handles_missing_field`）。
- 暂不设覆盖率阈值；重点覆盖公开行为，尤其是错误路径。

## 提交与拉取请求指南

使用 Conventional Commits 规范，提交信息使用中文：

- `feat:` 新功能、`fix:` 缺陷修复、`docs:` 文档、`refactor:` 无行为变化的重构、`test:` 仅测试、`chore:` 维护类变更。
- 主题行使用中文祈使句，不加句号，不超过 72 个字符，例如 `feat: 添加项目导出对话框`。
- 在正文中用中文说明变更背景，并引用关联 issue，例如 `Closes #12`。

拉取请求：

- 一个 PR 只做一个逻辑变更，目标分支为 `main`。
- 描述需用中文说明变更内容、原因与验证方式，并关联对应 issue。
- 保持差异可审阅（理想情况下不超过 400 行）；UI 变更需附截图。
- 请求评审前必须确保构建与测试通过。

## Agent 交互要求

- 全程使用中文思考和交流。
- 回答问题时避免过分的夸赞；任何判断都需反复推敲，优先保证准确性。
- 若信息或证据不足，主动向用户索要补充信息或证据，而非臆测。
- 回答保持结构化输出，条理清晰。
