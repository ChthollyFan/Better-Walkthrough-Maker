# 单元测试：目录结构镜像 src/，命名 test_<单元>_<行为>
find_package(Qt6 REQUIRED COMPONENTS Test)

set(TEST_SOURCES
  core/test_author_mark_style.cpp
  core/test_card_border.cpp
  core/test_image_fit.cpp
  core/test_page_background.cpp
  core/test_project_serialization.cpp
  core/test_project_manager.cpp
  core/test_article.cpp
  export/test_article_export.cpp
  export/test_export_renderer.cpp
  template/test_template.cpp
  ui/test_font_select_widget.cpp
  plugin/builtin/test_card_border_dialog.cpp
)

foreach(test_source IN LISTS TEST_SOURCES)
  get_filename_component(test_name ${test_source} NAME_WE)
  add_executable(${test_name} ${test_source})
  target_link_libraries(${test_name} PRIVATE Qt6::Test bwm_core)
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# 画布场景测试：画布代码属于主程序目标，这里额外编译 editor/ 下的两个源文件，
# 用「场景渲染成图片后取像素」的方式覆盖背景图 / 背景色 / 组件的层序。
add_executable(test_canvas_scene
  editor/test_canvas_scene.cpp
  ${CMAKE_SOURCE_DIR}/src/editor/CanvasScene.cpp
  ${CMAKE_SOURCE_DIR}/src/editor/ComponentItem.cpp
)
target_link_libraries(test_canvas_scene PRIVATE Qt6::Test bwm_core)
add_test(NAME test_canvas_scene COMMAND test_canvas_scene)

# 署名设置对话框测试：对话框代码属于主程序目标（src/app/），这里额外编译该源文件，
# 避免为测试把对话框下沉到 core（与 test_canvas_scene 的做法一致）。
add_executable(test_author_mark_dialog
  app/test_author_mark_dialog.cpp
  ${CMAKE_SOURCE_DIR}/src/app/dialogs/AuthorMarkDialog.cpp
)
target_link_libraries(test_author_mark_dialog PRIVATE Qt6::Test bwm_core)
add_test(NAME test_author_mark_dialog COMMAND test_author_mark_dialog)
