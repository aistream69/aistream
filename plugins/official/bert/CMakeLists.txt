
set(CMAKE_PREFIX_PATH "${PROJECT_ROOT_PATH}/work/3rdparty/libtorch")
find_package(Torch REQUIRED)

include_directories(
    )
link_directories(
    "${PROJECT_ROOT_PATH}/build/plugins/official/bert/ftokenizers"
    "${PROJECT_ROOT_PATH}/build/plugins/official/bert/tokenizers/tokenizers/lib/unilib"
    "${PROJECT_ROOT_PATH}/build/plugins/official/bert/tokenizers/tokenizers/bert"
    "${PROJECT_ROOT_PATH}/build/plugins/official/bert/tokenizers/tokenizers/utils"
    "${PROJECT_ROOT_PATH}/build/plugins/official/bert/tokenizers/tokenizers/fundamental"
    "${PROJECT_ROOT_PATH}/build/plugins/official/bert/tokenizers/tokenizers/basic"
    )

add_library(bertcpu SHARED bert_libtorch.cpp)

target_link_libraries(bertcpu
    ${TORCH_LIBRARIES} 
    -lftokenizers
    -lbert_tokenizer
    -lbasic_tokenizers
    -lfundamental_tokenizer
    -lutils
    -lunilib
    -licuuc
    -licui18n
    -Wl,-rpath,lib
    )

