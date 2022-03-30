
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    )

add_compile_options(-fPIC)

add_library(debug SHARED debug.cpp)

