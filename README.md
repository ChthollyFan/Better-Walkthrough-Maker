# Better-Walkthrough-Maker

**更好的攻略制作器** —— 面向游戏攻略作者的桌面设计工具：用"模板 + 自由画布"的方式制作攻略配图（装备推荐、属性对比、剧情流程等），导出 PNG 发布到小黑盒等平台。

## 文档

- [项目规划](docs/project-plan.md) — 产品定位、数据模型、功能模块、技术架构、里程碑与风险（开发前请先阅读）

## 构建与测试

依赖：CMake ≥ 3.21、Ninja、MinGW-w64 g++（14.x）、Qt 6（MinGW 版，如 6.11.2）。

```powershell
# 配置（显式指定 MinGW 版 Qt 路径；若环境变量 Qt6_DIR 指向其他版本需覆盖）
cmake -S . -B build -G Ninja "-DCMAKE_PREFIX_PATH=C:/Users/ThinkPad/Qt/6.11.2/mingw_64" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++

# 编译
cmake --build build

# 运行测试
$env:PATH = "C:/Users/ThinkPad/Qt/6.11.2/mingw_64/bin;" + $env:PATH
ctest --test-dir build --output-on-failure

# 运行主程序
build\src\bwm.exe
```

## 当前状态

- ✅ 规划定稿（`docs/project-plan.md`）
- ✅ M1 骨架：工程结构（`bwm_core` 静态库 + 主程序）、数据模型（项目 → 攻略 → 页面）、`project.json` 序列化（含容错）、项目管理（新建/打开/自动保存/崩溃恢复）、主窗口（菜单/项目树/画布占位）、单元测试全部通过
- ⏳ M2 画布：画布场景与组件（图片/文本/形状）、拖拽缩放旋转、图层、撤销重做
