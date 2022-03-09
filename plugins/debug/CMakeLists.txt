
include_directories(
    "${PROJECT_ROOT_PATH}/include"
    )

add_compile_options(-fPIC)

add_library(debug SHARED debug.cpp)

