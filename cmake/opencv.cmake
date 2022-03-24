include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(LIBOPENCV_PKG_DIR ${WORK_DIR}/pkg)
set(LIBOPENCV_DIR ${WORK_DIR}/3rdparty/opencv)
set(LIBOPENCV_SRC_DIR ${WORK_DIR}/3rdparty/opencv/opencv-4.5.5)

if(NOT EXISTS ${LIBOPENCV_DIR}/release)
    execute_process(COMMAND mkdir -p ${LIBOPENCV_DIR}/release)
endif()
if(NOT EXISTS ${LIBOPENCV_SRC_DIR})
    execute_process(COMMAND tar xzf ${LIBOPENCV_PKG_DIR}/opencv-4.5.5.tar.gz -C ${LIBOPENCV_DIR})
endif()

set(CONFIGURE_CMD cd ${LIBOPENCV_SRC_DIR} && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=RELEASE -DWITH_FFMPEG=ON -DCMAKE_INSTALL_PREFIX=${LIBOPENCV_DIR}/release ..)
set(BUILD_CMD cd ${LIBOPENCV_SRC_DIR}/build && make -j2)
set(INSTALL_CMD cd ${LIBOPENCV_SRC_DIR}/build && make install)

ExternalProject_Add(opencv
    PREFIX              opencv
    SOURCE_DIR          ${LIBOPENCV_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

