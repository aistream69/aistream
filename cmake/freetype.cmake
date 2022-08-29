include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(FREETYPE_PKG_DIR ${WORK_DIR}/pkg)
set(FREETYPE_DIR ${WORK_DIR}/3rdparty/freetype)
set(FREETYPE_SRC_DIR ${WORK_DIR}/3rdparty/freetype/freetype-2.10.0)

if(NOT EXISTS ${FREETYPE_DIR}/release)
    execute_process(COMMAND mkdir -p ${FREETYPE_DIR}/release)
endif()
if(NOT EXISTS ${FREETYPE_SRC_DIR})
    execute_process(COMMAND tar xf ${FREETYPE_PKG_DIR}/freetype-2.10.0.tar.gz -C ${FREETYPE_DIR})
endif()

set(CONFIGURE_CMD cd ${FREETYPE_SRC_DIR} && ./configure --prefix=${FREETYPE_DIR}/release)
set(BUILD_CMD cd ${FREETYPE_SRC_DIR} && make -j4)
set(INSTALL_CMD cd ${FREETYPE_SRC_DIR} && make install)

ExternalProject_Add(freetype
    PREFIX              freetype
    SOURCE_DIR          ${FREETYPE_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

