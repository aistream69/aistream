
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -O2 -rdynamic -Wno-deprecated-declarations -Wno-format-truncation")

set(CMAKE_VERBOSE_MAKEFILE ON)

include(${PROJECT_ROOT_PATH}/cmake/grpc_common.cmake)

# Proto file
get_filename_component(hw_proto "./protos/infer.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/infer.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/infer.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/infer.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/infer.grpc.pb.h")
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# hw_grpc_proto
add_library(hw_grpc_proto
  ${hw_grpc_srcs}
  ${hw_grpc_hdrs}
  ${hw_proto_srcs}
  ${hw_proto_hdrs})
target_link_libraries(hw_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

include_directories(
    "${PROJECT_ROOT_PATH}/include"
    )

add_library(triton SHARED server.cpp)
target_link_libraries(triton
  hw_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  -Wl,--disable-new-dtags,-rpath,${PROJECT_ROOT_PATH}/work/3rdparty/grpc/release/lib
  )

add_executable(test_async_client client.cpp)
target_link_libraries(test_async_client
  hw_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  -Wl,--disable-new-dtags,-rpath,${PROJECT_ROOT_PATH}/work/3rdparty/grpc/release/lib
  )

install (FILES 
    ${PROJECT_ROOT_PATH}/build/plugins/official/triton/libtriton.so
    DESTINATION plugins/official/triton)

