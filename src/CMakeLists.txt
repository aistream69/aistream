
set(target aistream)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -g -rdynamic")
set(CMAKE_C_FLAGS "-Wall -O2 -Wno-misleading-indentation")

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    )

link_directories(
    "${PROJECT_ROOT_PATH}/work/lib"
    )

add_executable(
    ${target} 
    main.cpp
    stream.cpp
    master.cpp
    slave.cpp
    config.cpp
    rest.cpp
    pipeline.cpp
    obj.cpp
    task.cpp
    share.cpp
    db.cpp
    output.cpp
    )

#add_dependencies(${target} ffmpeg)

set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_link_libraries(
    ${target}
    -lpthread
    -fPIE
    -Wl,-rpath,lib
    )

