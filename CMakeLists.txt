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
        server/server_.c
        server/server_.h
        test/server_test.c
        protocol/slenprotocol.c
        protocol/slenprotocol.c
        protocol/slenprotocol.h
        protocol/constant_.c
        protocol/constant_.h
        protocol/buffers_.c
        protocol/buffers_.h
        reply/reply_.c
        reply/reply_.h
        test/zero_cpy_test.c
        client/kvclient.c
        client/kvclient.h)

# 链接 uuid 库
target_link_libraries(kv_ ${UUID_LIBRARIES})
target_include_directories(kv_ PUBLIC ${UUID_INCLUDE_DIRS})