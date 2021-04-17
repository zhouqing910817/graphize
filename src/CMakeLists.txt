cmake_minimum_required(VERSION 2.8)
project(Main)
add_definitions(-c -g -pipe -pthread -fpermissive -std=c++11 -fno-omit-frame-pointer -IM)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/
                    /usr/local/include/
                   ${CMAKE_CURRENT_SOURCE_DIR}/pb_include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/brpc/include
                   ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fastcommon/include 
                   ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/shmcache/include
                   ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/gtest/include 
                  /usr/local/Cellar/openssl\@1.1/1.1.1k/include/
                  ${CMAKE_CURRENT_SOURCE_DIR}/proto
                  /usr/local/Cellar/openssl\@1.1/1.1.1i/include/
                  /usr/local/Cellar/openssl\@1.1/1.1.1j/include/
                  ${CMAKE_CURRENT_BINARY_DIR} #proto include file
)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/brpc/lib64
                 ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/shmcache/lib64
                 ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fastcommon/lib64
                  /usr/local/Cellar/openssl\@1.1/1.1.1k/lib/
                  /usr/local/lib
                  /usr/lib
                  /usr/local/Cellar/openssl\@1.1/1.1.1i/lib/
                  /usr/local/Cellar/openssl\@1.1/1.1.1j/lib/
)

set(CMAKE_EXE_LINKER_FLAGS "-Wl,-all_load")
add_compile_options(-fPIC)

find_package(Protobuf REQUIRED)
if (PROTOBUF_FOUND)
    message("protobuf found")
else ()
    message(FATAL_ERROR "Cannot find Protobuf")
endif ()
set(PROTO_FILES proto/echo.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
message("PROTO_SRCS = ${PROTO_SRCS}")
message("PROTO_HDRS = ${PROTO_HDRS}")

file(GLOB client_src "frame/client/*/*.cpp")
file(GLOB frame_src "frame/*.cpp")
file(GLOB common_src "common/*.cpp")
file(GLOB service_src "service/*.cpp")
file(GLOB service_base_src "service/base/*.cpp")
file(GLOB server_src "server/*.cpp")
add_library(frame STATIC
            ${client_src}
            ${frame_src}
            ${common_src}
            ${service_src}
            ${service_base_src}
            ${server_src}
            ${PROTO_SRCS}
            ${PROTO_HDRS}
)
add_executable(Main main.cpp)
target_link_libraries(frame -pthread -ldl -lz -lm -lssl -lcrypto -lprotobuf -lleveldb -lgflags -lsnappy  -lcpp-hocon -lboost_system -lboost_filesystem -lboost_locale -lboost_thread -lboost_regex  -lleatherman_util -lleatherman_locale -lbrpc
               ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/shmcache/lib64/libshmcache.a
               ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fastcommon/lib64/libfastcommon.a)
target_link_libraries(Main frame)

execute_process(COMMAND cp -r ../conf ${CMAKE_CURRENT_BINARY_DIR}/
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    )

message("result:${result}")
message("output:${output}")
# ut
file(GLOB GTEST_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/gtest/lib64/*.a)
add_executable(frame_ut 
               unittest/graph_frame/graph_test.cpp
)
target_link_libraries(frame_ut ${GTEST_LIBS} frame)
target_include_directories(frame_ut PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/gtest/include)
execute_process(COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/unittest/graph_frame
                COMMAND cp -r unittest/graph_frame/conf ${CMAKE_CURRENT_BINARY_DIR}/unittest/graph_frame/
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
