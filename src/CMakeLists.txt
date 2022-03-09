
set(target aistream)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O2 -g -rdynamic")
set(CMAKE_C_FLAGS "-Wall -O2 -Wno-misleading-indentation")

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(PLUGINS_OFFICIAL "${PROJECT_ROOT_PATH}/plugins/official")

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    "${PROJECT_ROOT_PATH}/src/backend/dylib"
    "${PLUGINS_OFFICIAL}/rtsp"
    "${PLUGINS_OFFICIAL}/gat1400"
    "${PROJECT_ROOT_PATH}/work/cjson/inc"
    "${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/include"
    )

link_directories(
    "${PROJECT_ROOT_PATH}/work/cjson/lib"
    "${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib"
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
    rest_slave.cpp
    pipeline.cpp
    obj.cpp
    task.cpp
    share.cpp
    db.cpp
    ../src/backend/dylib/dylib.cpp
    "${PLUGINS_OFFICIAL}/rtsp/rtsp.cpp"
    "${PLUGINS_OFFICIAL}/gat1400/gat1400.cpp"
    )

add_dependencies(${target} cjson)
add_dependencies(${target} libevent)

set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_link_libraries(
    ${target}
    -lcjson
    -levent_pthreads
    -levent
    -ldl
    -lpthread
    -fPIE
    -Wl,-rpath,lib
    )

install (TARGETS aistream DESTINATION .)
install (FILES 
    ${PROJECT_ROOT_PATH}/cfg/config.json  
    ${PROJECT_ROOT_PATH}/cfg/object.json  
    ${PROJECT_ROOT_PATH}/cfg/task.json 
    DESTINATION cfg)
install (FILES 
    ${PROJECT_ROOT_PATH}/samples/debug.json
    ${PROJECT_ROOT_PATH}/samples/jpegdec.json
    ${PROJECT_ROOT_PATH}/samples/simple.json
    DESTINATION samples)
install (FILES 
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent-2.1.so.7
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent-2.1.so.7.0.1
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent_pthreads.so
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent_pthreads-2.1.so.7
    ${PROJECT_ROOT_PATH}/work/3rdparty/libevent/release/lib/libevent_pthreads-2.1.so.7.0.1
    DESTINATION lib)

