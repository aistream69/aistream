include(ExternalProject)

set(WORK_DIR ${PROJECT_ROOT_PATH}/work)
set(FFMPEG_PKG_DIR ${WORK_DIR}/pkg)
set(FFMPEG_DIR ${WORK_DIR}/3rdparty/ffmpeg)
set(FFMPEG_SRC_DIR ${WORK_DIR}/3rdparty/ffmpeg/ffmpeg-3.4.9)

if(NOT EXISTS ${FFMPEG_DIR}/release)
    execute_process(COMMAND mkdir -p ${FFMPEG_DIR}/release)
endif()
if(NOT EXISTS ${FFMPEG_SRC_DIR})
    execute_process(COMMAND tar xf ${FFMPEG_PKG_DIR}/ffmpeg-3.4.9.tar.xz -C ${FFMPEG_DIR})
endif()

set(CONFIGURE_CMD cd ${FFMPEG_SRC_DIR} && ./configure --enable-shared --enable-gpl --enable-libx264 --extra-cflags=-I${WORK_DIR}/3rdparty/x264/release/include --extra-ldflags=-L${WORK_DIR}/3rdparty/x264/release/lib --prefix=${FFMPEG_DIR}/release)
set(BUILD_CMD cd ${FFMPEG_SRC_DIR} && make -j4)
set(INSTALL_CMD cd ${FFMPEG_SRC_DIR} && make install)

ExternalProject_Add(ffmpeg
    PREFIX              ffmpeg
    SOURCE_DIR          ${FFMPEG_PKG_DIR}
    CONFIGURE_COMMAND   ${CONFIGURE_CMD}
    BUILD_COMMAND       ${BUILD_CMD}
    INSTALL_COMMAND     ${INSTALL_CMD}
)

