include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(LIBTORCH_PKG_DIR ${WORK_DIR}/pkg)
set(LIBTORCH_DIR ${WORK_DIR}/3rdparty/libtorch)

if(NOT EXISTS ${LIBTORCH_DIR})
    execute_process(COMMAND tar xzf ${LIBTORCH_PKG_DIR}/libtorch-shared-with-deps-1.12.1-cpu.tar.gz -C ${WORK_DIR}/3rdparty)
endif()

set(CONFIGURE_CMD cd ${LIBTORCH_DIR})
set(BUILD_CMD cd ${LIBTORCH_DIR})
set(INSTALL_CMD cd ${LIBTORCH_DIR})

ExternalProject_Add(libtorch
    PREFIX              libtorch
    SOURCE_DIR          ${LIBTORCH_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

