
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    "${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/include/opencv4"
    )
link_directories(
    "${PROJECT_ROOT_PATH}/work/3rdparty/opencv/release/lib"
    )

add_compile_options(-fPIC)

add_library(detection SHARED detection.cpp)

add_dependencies(detection opencv)

target_link_libraries(detection
    -lopencv_highgui
    -lopencv_core
    -lopencv_imgproc
    -lopencv_imgcodecs
    -lopencv_dnn
    -Wl,-rpath,lib
    )

