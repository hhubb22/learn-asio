cmake_minimum_required(VERSION 3.15)

# 项目信息
project(hello_asio 
    VERSION 1.0.0
    DESCRIPTION "A simple C++ project with fmt and spdlog"
    LANGUAGES CXX
)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 配置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 设置构建类型默认值
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 导出 compile commands 用于 IDE 支持
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Conan 集成
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

# 查找依赖包
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(asio REQUIRED)

# 添加源码目录
add_subdirectory(src)