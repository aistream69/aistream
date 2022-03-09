
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -g -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    )

add_compile_options(-fPIC)

add_library(preview SHARED preview/preview.cpp)
add_library(rtsp SHARED rtsp/dl_rtsp.cpp)

