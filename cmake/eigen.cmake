include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(LIBEIGEN_PKG_DIR ${WORK_DIR}/pkg)
set(LIBEIGEN_DIR ${WORK_DIR}/3rdparty/eigen)
set(LIBEIGEN_SRC_DIR ${WORK_DIR}/3rdparty/eigen/eigen-3.3.9)

if(NOT EXISTS ${LIBEIGEN_DIR}/release)
    execute_process(COMMAND mkdir -p ${LIBEIGEN_DIR}/release)
endif()
if(NOT EXISTS ${LIBEIGEN_SRC_DIR})
    execute_process(COMMAND tar xzf ${LIBEIGEN_PKG_DIR}/eigen-3.3.9.tar.gz -C ${LIBEIGEN_DIR})
endif()

set(CONFIGURE_CMD cd ${LIBEIGEN_SRC_DIR} && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=${LIBEIGEN_DIR}/release ..)
set(BUILD_CMD cd ${LIBEIGEN_SRC_DIR})
set(INSTALL_CMD cd ${LIBEIGEN_SRC_DIR}/build && make install)

ExternalProject_Add(eigen
    PREFIX              eigen
    SOURCE_DIR          ${LIBEIGEN_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

