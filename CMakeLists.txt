cmake_minimum_required(VERSION 3.21)
# 版本号唯一来源：窗口/「关于」对话框显示的版本由 PROJECT_VERSION 注入主程序
# （见 src/CMakeLists.txt 的 BWM_VERSION 宏）。升版时同步更新 CHANGELOG.md。
project(BetterWalkthroughMaker VERSION 0.3.0 LANGUAGES CXX)

# 全局编译设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
  # 源码与字符串均使用 UTF-8，避免中文字面量乱码
  add_compile_options(/utf-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
