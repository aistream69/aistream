include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(RABBITMQ_PKG_DIR ${WORK_DIR}/pkg)
set(RABBITMQ_DIR ${WORK_DIR}/3rdparty/rabbitmq)
set(RABBITMQ_SRC_DIR ${WORK_DIR}/3rdparty/rabbitmq/rabbitmq-c-0.11.0)

if(NOT EXISTS ${RABBITMQ_DIR}/release)
    execute_process(COMMAND mkdir -p ${RABBITMQ_DIR}/release)
endif()
if(NOT EXISTS ${RABBITMQ_SRC_DIR})
    execute_process(COMMAND tar xzf ${RABBITMQ_PKG_DIR}/rabbitmq-c-0.11.0.tar.gz -C ${RABBITMQ_DIR})
endif()

set(CONFIGURE_CMD cd ${RABBITMQ_SRC_DIR} && mkdir -p build && cd build && cmake -DBUILD_TOOLS=OFF -DCMAKE_INSTALL_PREFIX=${RABBITMQ_DIR}/release ..)
set(BUILD_CMD cd ${RABBITMQ_SRC_DIR}/build && make -j4)
set(INSTALL_CMD cd ${RABBITMQ_SRC_DIR}/build && make install)

ExternalProject_Add(rabbitmq
    PREFIX              rabbitmq
    SOURCE_DIR          ${RABBITMQ_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

