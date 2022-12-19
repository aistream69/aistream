
# because tokenizers is conflict with libpytorch which use -D_GLIBCXX_USE_CXX11_ABI=0,
# so make a new CMakeLists.txt project

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -O2 -rdynamic -Wno-reorder")

include_directories(
    "${PROJECT_ROOT_PATH}/plugins/official/bert/tokenizers"
    )

add_compile_options(-fPIC)

add_library(ftokenizers STATIC ftokenizers.cpp)

