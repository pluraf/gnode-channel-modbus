cmake_minimum_required(VERSION 3.25)

project(ChannelModbus C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

option(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# Proto file
get_filename_component(api_proto "./protos/channel_modbus_api.proto" ABSOLUTE)
get_filename_component(api_proto_path "${api_proto}" PATH)

# Generated sources
set(api_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/channel_modbus_api.pb.cc")
set(api_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/channel_modbus_api.pb.h")
set(api_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/channel_modbus_api.grpc.pb.cc")
set(api_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/channel_modbus_api.grpc.pb.h")

add_custom_command(
      OUTPUT "${api_proto_srcs}" "${api_proto_hdrs}" "${api_grpc_srcs}" "${api_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${api_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${api_proto}"
      DEPENDS "${api_proto}")

include_directories("${CMAKE_CURRENT_BINARY_DIR}" "/usr/include/modbus")

add_executable(gnode-channel-modbus
    ${api_grpc_srcs}
    ${api_grpc_hdrs}
    ${api_proto_srcs}
    ${api_proto_hdrs}
    src/server.cpp
)

target_link_libraries(gnode-channel-modbus
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    modbus
)
