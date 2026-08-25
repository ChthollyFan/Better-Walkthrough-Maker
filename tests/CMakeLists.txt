# 单元测试：目录结构镜像 src/，命名 test_<单元>_<行为>
find_package(Qt6 REQUIRED COMPONENTS Test)

set(TEST_SOURCES
  core/test_project_serialization.cpp
  core/test_project_manager.cpp
  export/test_export_renderer.cpp
  template/test_template.cpp
)

foreach(test_source IN LISTS TEST_SOURCES)
  get_filename_component(test_name ${test_source} NAME_WE)
  add_executable(${test_name} ${test_source})
  target_link_libraries(${test_name} PRIVATE Qt6::Test bwm_core)
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
