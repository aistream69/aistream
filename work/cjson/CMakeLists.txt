
execute_process(COMMAND mkdir -p ${PROJECT_ROOT_PATH}/work/cjson/lib)
include_directories(
    "inc"
    )

add_compile_options(-fPIC)

add_library(cjson STATIC src/cJSON.c)

set(LIBRARY_OUTPUT_PATH ${PROJECT_ROOT_PATH}/work/cjson/lib)

