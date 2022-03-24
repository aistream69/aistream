include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(RTSP_PKG_DIR ${WORK_DIR}/pkg)
set(RTSP_DIR ${WORK_DIR}/3rdparty/rtsp)
set(RTSP_SRC_DIR ${WORK_DIR}/3rdparty/rtsp/rtsp-client)

if(NOT EXISTS ${RTSP_DIR}/release)
    execute_process(COMMAND mkdir -p ${RTSP_DIR}/release/inc)
    execute_process(COMMAND mkdir -p ${RTSP_DIR}/release/lib)
endif()
if(NOT EXISTS ${RTSP_SRC_DIR})
    execute_process(COMMAND tar xzf ${RTSP_PKG_DIR}/rtsp-client.tar.gz -C ${RTSP_DIR})
endif()

set(CONFIGURE_CMD echo "do nothing")
set(BUILD_CMD cd ${RTSP_SRC_DIR}/player && make -j4)
set(INSTALL_CMD echo "do nothing")

ExternalProject_Add(rtsplib
    PREFIX              rtsplib
    SOURCE_DIR          ${RTSP_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

