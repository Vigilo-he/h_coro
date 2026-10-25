cmake_minimum_required(VERSION 3.10)
project(h_coro)

# 强制使用 C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

# 因为 Task 是模板类（全写在 .h 里），所以这里只需要建一个假源文件让 CMake 满意，
# 或者如果你以后有非模板的 .cc 也可以加进来。
add_library(h_coro INTERFACE)
# INTERFACE 意味着这是一个纯头文件库，没有编译产物，只有头文件路径。
target_include_directories(h_coro INTERFACE ${PROJECT_SOURCE_DIR}/include)