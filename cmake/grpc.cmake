include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(LIBGRPC_PKG_DIR ${WORK_DIR}/pkg)
set(LIBGRPC_DIR ${WORK_DIR}/3rdparty/grpc)
set(LIBGRPC_SRC_DIR ${WORK_DIR}/3rdparty/grpc/grpc)

if(NOT EXISTS ${LIBGRPC_DIR}/release)
    execute_process(COMMAND mkdir -p ${LIBGRPC_DIR}/release)
endif()
if(NOT EXISTS ${LIBGRPC_SRC_DIR})
    execute_process(COMMAND tar xzf ${LIBGRPC_PKG_DIR}/grpc-1.50.0.tar.gz -C ${LIBGRPC_DIR})
endif()

set(CONFIGURE_CMD cd ${LIBGRPC_SRC_DIR} && mkdir -p build && cd build && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=${LIBGRPC_DIR}/release ..)
set(BUILD_CMD cd ${LIBGRPC_SRC_DIR}/build && make -j4)
set(INSTALL_CMD cd ${LIBGRPC_SRC_DIR}/build && make install)

ExternalProject_Add(grpc
    PREFIX              grpc
    SOURCE_DIR          ${LIBGRPC_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

