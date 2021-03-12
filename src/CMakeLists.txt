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
                  /usr/local/Cellar/openssl\@1.1/1.1.1j/include/
                  ${CMAKE_CURRENT_SOURCE_DIR}/proto
)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/brpc/lib64
                 ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/shmcache/lib64
                 ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fastcommon/lib64
                  /usr/local/Cellar/openssl\@1.1/1.1.1j/lib/
                  /usr/local/lib
                 /usr/lib
)
file(GLOB client_src "frame/client/*/*.cpp")
file(GLOB frame_src "frame/*.cpp")
file(GLOB common_src "common/*.cpp")
file(GLOB service_src "service/*.cpp")
file(GLOB service_base_src "service/base/*.cpp")
file(GLOB server_src "server/*.cpp")
file(GLOB proto_src "proto/*.cc")
add_library(frame STATIC
            ${client_src}
            ${frame_src}
            ${common_src}
            ${service_src}
            ${service_base_src}
            ${server_src}
            ${proto_src}
)
add_executable(Main main.cpp)
target_link_libraries(frame -pthread -ldl -lz -lm -lssl -lcrypto -lprotobuf -lleveldb -lgflags -lsnappy  -lcpp-hocon -lboost_system -lboost_filesystem -lboost_locale-mt -lboost_thread-mt -lboost_regex  -lleatherman_util -lleatherman_locale -lbrpc 
               ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/shmcache/lib64/libshmcache.a
               ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fastcommon/lib64/libfastcommon.a)
target_link_libraries(Main frame)

# ut
file(GLOB GTEST_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/gtest/lib64/*.a)
file(GLOB frame_ut_src "unittest/graph_frame/src/*.cpp")
add_executable(frame_ut 
               unittest/graph_frame/graph_test.cpp
               ${frame_ut_src}
)
target_link_libraries(frame_ut ${GTEST_LIBS} frame)
