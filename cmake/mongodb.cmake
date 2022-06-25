include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(LIBMONGODB_PKG_DIR ${WORK_DIR}/pkg)
set(LIBMONGODB_DIR ${WORK_DIR}/3rdparty/mongodb)
set(LIBMONGODB_SRC_DIR ${WORK_DIR}/3rdparty/mongodb/mongo-c-driver-1.20.1)

if(NOT EXISTS ${LIBMONGODB_DIR}/release)
    execute_process(COMMAND mkdir -p ${LIBMONGODB_DIR}/release)
endif()
if(NOT EXISTS ${LIBMONGODB_SRC_DIR})
    execute_process(COMMAND tar xzf ${LIBMONGODB_PKG_DIR}/mongo-c-driver-1.20.1.tar.gz -C ${LIBMONGODB_DIR})
endif()

set(CONFIGURE_CMD cd ${LIBMONGODB_SRC_DIR} && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=${LIBMONGODB_DIR}/release ..)
set(BUILD_CMD cd ${LIBMONGODB_SRC_DIR}/build && make -j4)
set(INSTALL_CMD cd ${LIBMONGODB_SRC_DIR}/build && make install)

ExternalProject_Add(mongodb
    PREFIX              mongodb
    SOURCE_DIR          ${LIBMONGODB_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

