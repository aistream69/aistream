
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -O2 -rdynamic -Wno-deprecated-declarations -Wno-format-truncation -Wno-sign-compare -Wno-unused-result")

include_directories(
    "${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/include"
    )
link_directories(
    "${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/x264/release/lib"
    )

add_compile_options(-fPIC)

add_library(common SHARED 
    common.cpp 
    )

add_dependencies(common cjson ffmpeg)

target_link_libraries(common
    -lavformat 
    -lavfilter 
    -lavcodec 
    -lavutil 
    -Wl,-rpath,lib
    )

set(LIBRARY_OUTPUT_PATH ${PROJECT_ROOT_PATH}/plugins/lib)

