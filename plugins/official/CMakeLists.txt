
include_directories(
    "${PROJECT_ROOT_PATH}/include"
    )

add_compile_options(-fPIC)

add_library(preview SHARED preview/preview.cpp)

#set(LIBRARY_OUTPUT_PATH ${PROJECT_ROOT_PATH}/work/slab/lib)

