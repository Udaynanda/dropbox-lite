cmake_minimum_required(VERSION 3.20)
project(DropboxLite VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Try to find gRPC and Protobuf
find_package(gRPC CONFIG)
find_package(Protobuf CONFIG)

# If not found, we'll build without gRPC for now (benchmarks don't need it)
if(NOT gRPC_FOUND OR NOT Protobuf_FOUND)
    message(WARNING "gRPC or Protobuf not found. Building without network components.")
    set(BUILD_NETWORK OFF)
else()
    set(BUILD_NETWORK ON)
endif()

# SQLite3
find_package(SQLite3)
if(NOT SQLite3_FOUND)
    message(WARNING "SQLite3 not found. Some features will be disabled.")
    set(BUILD_DATABASE OFF)
else()
    set(BUILD_DATABASE ON)
endif()

# Proto files (only if gRPC is available)
if(BUILD_NETWORK)
    set(PROTO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/proto")
    set(PROTO_FILES ${PROTO_PATH}/sync.proto)
    
    set(PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/sync.pb.cc")
    set(PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/sync.pb.h")
    set(GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/sync.grpc.pb.cc")
    set(GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/sync.grpc.pb.h")
    
    add_custom_command(
        OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
        COMMAND protobuf::protoc
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
             --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
             -I "${PROTO_PATH}"
             --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
             ${PROTO_FILES}
        DEPENDS ${PROTO_FILES}
    )
else()
    set(PROTO_SRCS)
    set(GRPC_SRCS)
endif()

# Common library (core functionality without network)
add_library(dropbox_common STATIC
    src/common/hash.cpp
    src/common/chunker.cpp
    src/common/compression.cpp
    src/common/logger.cpp
    src/common/metrics.cpp
    src/common/rate_limiter.cpp
    src/common/thread_pool.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_include_directories(dropbox_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(dropbox_common PUBLIC
    OpenSSL::Crypto
    Threads::Threads
)

if(BUILD_NETWORK)
    target_link_libraries(dropbox_common PUBLIC
        gRPC::grpc++
        protobuf::libprotobuf
    )
endif()

# Core library (only if database is available)
if(BUILD_DATABASE)
    add_library(dropbox_core STATIC
        src/core/metadata_db.cpp
        src/core/delta_engine.cpp
        src/core/conflict_resolver.cpp
    )
    
    target_include_directories(dropbox_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(dropbox_core PUBLIC
        dropbox_common
        SQLite::SQLite3
    )
endif()

# Client and Server libraries (only if network and database available)
if(BUILD_NETWORK AND BUILD_DATABASE)
    add_library(dropbox_client STATIC
        src/client/file_watcher.cpp
        src/client/sync_engine.cpp
        src/client/upload_manager.cpp
    )
    
    target_include_directories(dropbox_client PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(dropbox_client PUBLIC
        dropbox_core
    )
    
    add_library(dropbox_server STATIC
        src/server/storage_manager.cpp
        src/server/sync_service.cpp
    )
    
    target_include_directories(dropbox_server PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    target_link_libraries(dropbox_server PUBLIC
        dropbox_core
    )
    
    # Client executable
    add_executable(dropbox_client_app
        src/client/main.cpp
    )
    
    target_link_libraries(dropbox_client_app PRIVATE
        dropbox_client
    )
    
    # Server executable
    add_executable(dropbox_server_app
        src/server/main.cpp
    )
    
    target_link_libraries(dropbox_server_app PRIVATE
        dropbox_server
    )
endif()

# Enable testing
enable_testing()
add_subdirectory(tests)

# Add benchmarks
add_subdirectory(benchmarks)
