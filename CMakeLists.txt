cmake_minimum_required(VERSION 3.30)
project(kv_ C)

set(CMAKE_C_STANDARD 11)

# 使用 pkg-config 查找 uuid
find_package(PkgConfig REQUIRED)
pkg_check_modules(UUID REQUIRED uuid)

add_executable(kv_ main.c
        core/k_v.c
        core/k_v.h
        commands/command_.c
        commands/command_.h
        test/k_v_test.c
        test/command_test.c)

# 链接 uuid 库
target_link_libraries(kv_ ${UUID_LIBRARIES})
target_include_directories(kv_ PUBLIC ${UUID_INCLUDE_DIRS})