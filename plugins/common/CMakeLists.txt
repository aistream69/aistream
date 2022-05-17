
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -rdynamic -Wno-deprecated-declarations -Wno-format-truncation -Wno-sign-compare -Wno-unused-result")

include_directories(
    "${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/include"
    )
link_directories(
    "${PROJECT_ROOT_PATH}/work/3rdparty/ffmpeg/release/lib"
    )

add_compile_options(-fPIC)

add_library(common SHARED 
    common.cpp 
    )

add_dependencies(common ffmpeg)

target_link_libraries(common
    -lavformat 
    -lavfilter 
    -lavcodec 
    -lavutil 
    )

set(LIBRARY_OUTPUT_PATH ${PROJECT_ROOT_PATH}/plugins/lib)

