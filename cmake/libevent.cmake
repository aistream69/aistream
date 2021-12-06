include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(LIBEVENT_PKG_DIR ${WORK_DIR}/pkg)
set(LIBEVENT_DIR ${WORK_DIR}/3rdparty/libevent)
set(LIBEVENT_SRC_DIR ${WORK_DIR}/3rdparty/libevent/libevent-2.1.12-stable)

if(NOT EXISTS ${LIBEVENT_DIR}/release)
    execute_process(COMMAND mkdir -p ${LIBEVENT_DIR}/release)
endif()
if(NOT EXISTS ${LIBEVENT_SRC_DIR})
    execute_process(COMMAND tar xzf ${LIBEVENT_PKG_DIR}/libevent-2.1.12-stable.tar.gz -C ${LIBEVENT_DIR})
endif()

set(CONFIGURE_CMD cd ${LIBEVENT_SRC_DIR} && ./configure --prefix=${LIBEVENT_DIR}/release)
set(BUILD_CMD cd ${LIBEVENT_SRC_DIR} && make -j4)
set(INSTALL_CMD cd ${LIBEVENT_SRC_DIR} && make install)

ExternalProject_Add(libevent
    PREFIX              libevent
    SOURCE_DIR          ${LIBEVENT_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

