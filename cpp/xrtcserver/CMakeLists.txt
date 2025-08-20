cmake_minimum_required(VERSION 3.10)
project(xrtcserver)

# 移除 -fPIC 选项（可执行文件不需要）实际情况应重新编译第三方库来取消此选项
add_definitions("-g -pipe -W -Wall -std=gnu++14 -DWEBRTC_POSIX -DWEBRTC_LINUX")

include_directories(
    "./src"
    "./3rdlibs/yaml-cpp/include"
    "./3rdlibs/libev/build/include"
    "../rtcbase/src"
    "../rtcbase/third_party/include"
)

link_directories(
    "./3rdlibs/yaml-cpp/lib"
    "./3rdlibs/libev/build/lib"
    "../rtcbase/out"
    "../rtcbase/third_party/lib"
)

file(GLOB all_src 
            "./src/*.cpp"
            "./src/base/*.cpp"
            "./src/server/*.cpp"
            )

add_executable(xrtcserver ${all_src})

# 仅使用 -fno-PIE 和 -no-pie（移除 -nopie） 仅在本地测试时开启，实际情况应重新编译第三方库来取消此选项
target_compile_options(xrtcserver PRIVATE -fno-PIE)

# 使用更兼容的 LINK_FLAGS 设置方式
set_target_properties(xrtcserver PROPERTIES
    LINK_FLAGS "-no-pie"
)

target_link_libraries(xrtcserver 
    libyaml-cpp.a 
    librtcbase.a 
    libabsl_strings.a
    libabsl_throw_delegate.a 
    libev.a
    -lpthread
)