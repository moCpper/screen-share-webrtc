cmake_minimum_required (VERSION 3.5)

project(rtcbase)

include_directories(
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/third_party/include"
)

link_directories(
    "${PROJECT_SOURCE_DIR}/third_party/lib"
)

add_definitions("-g -pipe -W -Wall -fPIC -std=gnu++14 -DWEBRTC_POSIX
    -DWEBRTC_LINUX -DNDEBUG -Wno-unused-parameter")

file(GLOB all_src
    "./src/rtc_base/*.cc"
    "./src/rtc_base/strings/*.cc"
    "./src/rtc_base/synchronization/*.cc"
    "./src/rtc_base/deprecated/*.cc"
    "./src/rtc_base/internal/*.cc"
    "./src/rtc_base/task_utils/*.cc"
    "./src/rtc_base/third_party/sigslot/*.cc"
    "./src/rtc_base/third_party/base64/*.cc"
    "./src/api/task_queue/*.cc"
    "./src/api/units/*.cc"
    "./src/api/crypto/*.cc"
    "./src/system_wrappers/source/*.cc"
)

add_library(rtcbase ${all_src})

target_link_libraries(rtcbase libssl.a libcrypto.a)


