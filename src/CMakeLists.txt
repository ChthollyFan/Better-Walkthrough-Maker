# 核心库：数据模型、序列化、项目管理、设置、渲染与导出、模板与主题、
# 插件接口与内置适配器（主程序与单元测试共用）
add_library(bwm_core STATIC
  core/Article.cpp
  core/Component.cpp
  core/ComponentPainter.cpp
  core/ComponentSerializer.cpp
  core/Project.cpp
  core/ProjectSerializer.cpp
  core/Template.cpp
  export/ArticleImporter.cpp
  export/ArticleRenderer.cpp
  export/ExportRenderer.cpp
  export/ExportResultHelper.cpp
  project/ProjectManager.cpp
  settings/Settings.cpp
  template/BuiltinTemplates.cpp
  template/TemplateManager.cpp
  theme/Theme.cpp
  # UI 外观层（窗口/控件风格）
  ui/AcrylicHelper.cpp
  ui/GlassStyleProvider.cpp
  ui/UiStyle.cpp
  # 插件接口与宿主
  plugin/PluginHost.cpp
  # 内置适配器
  plugin/builtin/BuiltinComponentProviders.cpp
  plugin/builtin/BuiltinArticleExportProviders.cpp
  plugin/builtin/BuiltinExportProviders.cpp
  plugin/builtin/BuiltinTemplateProviders.cpp
  plugin/builtin/BuiltinThemeProviders.cpp
  plugin/builtin/BuiltinPluginRegistrar.cpp
)
target_include_directories(bwm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bwm_core PUBLIC Qt6::Widgets Qt6::PrintSupport)
if(WIN32)
    # AcrylicHelper 调用 DWM API（DwmSetWindowAttribute 等）
    target_link_libraries(bwm_core PUBLIC dwmapi)
endif()

# 主程序：仅包含应用层（入口与主窗口、面板、对话框、命令）。
# WIN32 关键字使程序以 Windows GUI 子系统链接，双击启动时不再弹出控制台窗口。
add_executable(bwm WIN32
  ${CMAKE_SOURCE_DIR}/resources/resources.qrc
  app/main.cpp
  app/MainWindow.cpp
  app/ComponentDisplay.cpp
  app/commands/PageSnapshotCommand.cpp
  app/panels/ArticleEditor.cpp
  app/panels/MarkdownPreview.cpp
  app/panels/ProjectTreePanel.cpp
  app/panels/AssetPanel.cpp
  app/panels/LayerPanel.cpp
  app/dialogs/NewProjectDialog.cpp
  app/dialogs/ExportDialog.cpp
  app/dialogs/SettingsDialog.cpp
  editor/CanvasScene.cpp
  editor/CanvasView.cpp
  editor/ComponentItem.cpp
)

# Windows 资源脚本：把应用图标嵌入 exe（资源管理器/任务栏显示）
if(WIN32)
    target_sources(bwm PRIVATE ${CMAKE_SOURCE_DIR}/resources/app.rc)
endif()

target_link_libraries(bwm PRIVATE bwm_core)
