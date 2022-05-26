include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(X264_PKG_DIR ${WORK_DIR}/pkg)
set(X264_DIR ${WORK_DIR}/3rdparty/x264)
set(X264_SRC_DIR ${WORK_DIR}/3rdparty/x264/x264-20191217-2245)

if(NOT EXISTS ${X264_DIR}/release)
    execute_process(COMMAND mkdir -p ${X264_DIR}/release)
endif()
if(NOT EXISTS ${X264_SRC_DIR})
    execute_process(COMMAND tar xf ${X264_PKG_DIR}/x264-20191217-2245.tar.gz -C ${X264_DIR})
endif()

set(CONFIGURE_CMD cd ${X264_SRC_DIR} && ./configure --enable-shared --prefix=${X264_DIR}/release)
set(BUILD_CMD cd ${X264_SRC_DIR} && make -j4)
set(INSTALL_CMD cd ${X264_SRC_DIR} && make install)

ExternalProject_Add(x264
    PREFIX              x264
    SOURCE_DIR          ${X264_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

