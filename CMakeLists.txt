cmake_minimum_required(VERSION 3.21)
project(BetterWalkthroughMaker VERSION 0.1.0 LANGUAGES CXX)

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

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
