# Repository Guidelines

## 交互要求

- 全程使用中文思考和交流。
- 回答问题时避免过分的夸赞；任何判断都需反复推敲，优先保证准确性。
- 若信息或证据不足，主动向用户索要补充信息或证据，而非臆测。
- 回答保持结构化输出，条理清晰。
- 任何 **git 提交（commit）** 之前，必须先向用户列出提交方案（涉及文件、提交信息），并使用 `ask_user_question` 询问是否提交，获得用户明确确认后方可执行。
- 任何 **发布操作（打 tag / 推送 tag / 创建 Release）** 之前，同样必须先列出方案并使用 `ask_user_question` 询问用户是否执行，确认后方可执行。
- 此规则对普通提交与文档改动一视同仁，用户未确认前不得执行 git 写操作。

## 编码规范

参考 /docs/C++语言编码规范.md

## 项目结构与模块组织

当前结构（M1 骨架已落地）：

- `src/app/` — 入口与主窗口。
- `src/core/` — 数据模型与 `project.json` 序列化。
- `src/project/` — 项目管理（新建/打开/自动保存/崩溃恢复）。
- `src/settings/` — 全局设置（QSettings）。
- `tests/` — 单元测试（Qt Test），目录结构镜像 `src/`。
- `docs/` — 项目规划等文档。

约定：源码放在 `src/` 按功能模块组织；测试放在 `tests/` 镜像 `src/`；其他文档放在 `docs/`。核心库目标为 `bwm_core`（静态库），主程序与测试共用。

## 插件架构

本项目以**动态库运行时加载**为最终插件化目标，当前处于**编译时静态注册**阶段，已预留升级路径。

### 扩展点

所有扩展点通过统一的 `PluginHost`（`src/plugin/PluginHost.h`）注册与查询。当前已定义 5 类接口：

| 扩展点 | 接口 | 头文件 | 作用 |
| --- | --- | --- | --- |
| 组件类型 | `IComponentProvider` | `src/plugin/IComponentProvider.h` | 注册新的画布组件类型，自动出现在"插入"菜单和工具栏 |
| 导出格式 | `IExportProvider` | `src/plugin/IExportProvider.h` | 注册新的导出格式，自动出现在导出对话框格式下拉框 |
| 面板/视图 | `IPanelProvider` | `src/plugin/IPanelProvider.h` | 注册新的侧边面板，自动以标签页挂载到主界面 |
| 模板包 | `ITemplateProvider` | `src/plugin/ITemplateProvider.h` | 提供模板集合，合并到"新建攻略"模板选择列表 |
| 主题 | `IThemeProvider` | `src/plugin/IThemeProvider.h` | 提供主题配色，合并到"主题"菜单 |

### 当前状态

- 内置功能已适配为插件实现，位于 `src/plugin/builtin/`，由 `registerBuiltinPlugins()` 在 `MainWindow` 构造时统一注册。
- `MainWindow` 的菜单、工具栏、导出对话框等均从 `PluginHost` 动态查询扩展点列表构建，新增插件后无需修改框架代码即可自动出现。
- `PluginHost` 中预留 `loadPlugins()` 方法（TODO），未来扫描 `plugins/` 目录通过 `QPluginLoader` 动态加载动态库。

### 插件开发流程文档

开发各类插件时，参考以下文档（当用户要求开发某类插件时，应先指向对应文档并与用户讨论细节）：

- 组件类型插件：`docs/plugin-component-dev.md`
- 导出格式插件：`docs/plugin-export-dev.md`
- 面板/视图插件：`docs/plugin-panel-dev.md`
- 模板包插件：`docs/plugin-template-dev.md`
- 主题插件：`docs/plugin-theme-dev.md`

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

# 部署 Qt 运行时 DLL 到 exe 同目录（首次编译或清除 build 后需执行一次）
C:\Users\ThinkPad\Qt\6.11.2\mingw_64\bin\windeployqt.exe --release --compiler-runtime build\src\bwm.exe
```

> **注意**：`windeployqt` 会把 Qt6Core/Qt6Widgets/Qt6Gui 等运行时 DLL 及平台插件（`platforms\qwindows.dll`）复制到 `build\src\` 下。清除 `build` 目录重新配置后这些 DLL 会丢失，需重新执行上述部署命令。仅设置 `PATH` 指向 Qt 的 `bin` 目录也可以运行，但双击 exe 时找不到 DLL，推荐用 `windeployqt` 做自包含部署。

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

## Tag 发布流程

> 说明：本流程**仅在用户明确提出发布要求时执行**，日常开发中不执行。
> 本项目适配：第五步打 tag 并推送后，GitHub Actions（`.github/workflows/release.yml`）会自动完成构建、测试、打包并发布到 GitHub Releases，故第六节"部署"中的手动服务器部署对本项目不适用（无服务器，改为验证 Release 产物）。

---

### 一、代码冻结

1. 切换至 dev 分支：
   ```
   git checkout dev
   ```
2. 确认所有待发布功能已合并至 dev。
3. 执行本地全量测试（单元测试、集成测试、冒烟测试）。
4. 测试未通过则修复后重复步骤 3。
5. 更新相关文档（README、部署配置、接口文档）。

---

### 二、版本号拟定

1. 获取当前版本号（从上一个 Tag 或 `package.json`/`pom.xml` 等文件读取）。
2. 依据语义化版本规范确定新版本号：

| 变更类型 | 版本号调整 |
| :--- | :--- |
| 存在不兼容的 API 破坏性变更 | 主版本号 +1，次版本号归零，补丁归零 |
| 存在向下兼容的新功能 | 次版本号 +1，补丁归零 |
| 仅修复 Bug，无新功能 | 补丁版本号 +1 |
| 预发布版本 | 正式版本后追加 `-alpha.n` / `-beta.n` / `-rc.n` |

3. 记录新版本号（例如 `1.1.0`）。

---

### 三、生成更新日志

1. 获取上一个 Tag 名称（例如 `v1.0.0`）：
   ```
   git describe --tags --abbrev=0
   ```
2. 提取两个 Tag 之间所有提交：
   ```
   git log --pretty=format:"- %s" --no-merges v1.0.0..dev
   ```
3. 将提交按以下分类归纳：
   - Added：新功能
   - Fixed：Bug 修复
   - Changed：重构、优化、依赖升级
   - Deprecated：标记弃用
   - Removed：移除功能
   - Security：安全修复
4. 将归纳内容写入 `CHANGELOG.md` 文件顶部：

   ```
   ## [1.1.0] - 2026-08-28

   ### Added
   - 新增数据导出功能

   ### Fixed
   - 修复日期筛选器无效问题

   ### Changed
   - 优化列表页加载速度

   ### 升级影响
   - 向下兼容：是
   - 如需手动操作：无

   ### 已知问题
   - 无
   ```
5. 提交 CHANGELOG.md：
   ```
   git add CHANGELOG.md
   git commit -m "docs: 更新 CHANGELOG 至 v1.1.0"
   ```

---

### 四、合并至主分支

1. 切换到 main 分支：
   ```
   git checkout main
   ```
2. 拉取最新远程 main 分支（如有协作）：
   ```
   git pull origin main
   ```
3. 合并 dev 分支（使用 `--no-ff` 保留合并记录）：
   ```
   git merge --no-ff dev -m "chore: release v1.1.0"
   ```
4. 解决冲突（如有）后提交。

---

### 五、打 Tag

1. 创建附注标签，正文包含完整变更信息：
   ```
   git tag -a v1.1.0
   ```
2. 在编辑器中填入以下内容：

   ```
   ## [1.1.0] - 2026-08-28

   ### Added
   - 新增数据导出功能，支持 CSV 和 Excel 格式

   ### Fixed
   - 修复日期范围筛选器跨月查询错误
   - 修复导出文件中文乱码

   ### Changed
   - 优化列表页加载速度，首屏时间减少约 30%
   - 升级 Spring Boot 从 2.7.0 至 2.7.5

   ### Deprecated
   - 旧版 /api/v1/report 接口将于 v2.0.0 移除

   ### Security
   - 升级 log4j 至 2.17.2

   ### 升级影响
   - 向下兼容：是
   - 如需手动操作：无

   ### 已知问题
   - 导出 Excel 超过 5000 行时内存占用偏高

   ### 校验信息
   - 构建哈希: 7a3f8e2
   ```
3. 推送 main 分支及 Tag 至远程：
   ```
   git push origin main --tags
   ```

---

### 六、部署

1. 从 main 分支构建生产环境包。
2. 部署至生产服务器。
3. 执行线上冒烟测试（核心功能验证）。
4. 测试通过则继续；不通过则执行回滚（步骤八）。

> 本项目适配：GitHub Actions 自动构建并发布 Release，本步改为验证 Release 产物的构建与下载可用性。

---

### 七、发布后收尾

1. 切换回 dev 分支：
   ```
   git checkout dev
   ```
2. 将 main 合并回 dev（保持同步）：
   ```
   git merge main
   ```
3. 推送 dev 分支：
   ```
   git push origin dev
   ```
4. 继续日常开发。

---

### 八、回滚流程

仅在部署后验证失败时执行：

1. 查看历史 Tag：
   ```
   git tag --list
   ```
2. 回滚 main 分支至上一个稳定 Tag（例如 `v1.0.0`）：
   ```
   git checkout main
   git reset --hard v1.0.0
   git push origin main --force
   ```
3. 重新部署并验证。
4. 修复 dev 分支问题后，重新执行流程。

---

### 九、检查清单

发布前逐项确认：

- [ ] 本地所有测试通过
- [ ] CHANGELOG.md 已更新并提交
- [ ] 文档（README、配置）已同步更新
- [ ] 版本号符合语义化版本规范
- [ ] Tag 正文包含完整的变更分类、升级影响、已知问题
- [ ] Tag 正文内容与 CHANGELOG.md 对应条目一致
- [ ] main 分支已包含所有待发布代码
- [ ] 远程推送成功（`git push --tags`）

