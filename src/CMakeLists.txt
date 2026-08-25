# 核心库：数据模型、序列化、项目管理、设置、渲染与导出（主程序与单元测试共用）
add_library(bwm_core STATIC
  core/Component.cpp
  core/ComponentPainter.cpp
  core/Project.cpp
  core/ProjectSerializer.cpp
  export/ExportRenderer.cpp
  project/ProjectManager.cpp
  settings/Settings.cpp
)
target_include_directories(bwm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bwm_core PUBLIC Qt6::Widgets)

# 主程序：仅包含应用层（入口与主窗口）
add_executable(bwm
  app/main.cpp
  app/MainWindow.cpp
  editor/CanvasScene.cpp
  editor/CanvasView.cpp
  editor/ComponentItem.cpp
)

target_link_libraries(bwm PRIVATE bwm_core)
