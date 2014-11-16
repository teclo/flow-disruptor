cmake_minimum_required(VERSION 2.8)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -Werror -fno-strict-aliasing")

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS
                      proto/FlowDisruptorConfig.proto
                      proto/PacketHeader.proto)

find_library(PCAP pcap)
find_library(GFLAGS gflags)
find_library(EV ev)

add_executable(flow-disruptor 
               src/main.cc
               src/bpf.cc
               src/config.cc
               src/connection.cc
               src/connection-table.cc
               src/io-backend.cc
               src/io-backend-pcap.cc
               src/log.cc
               src/packet.cc
               src/pcap-dumper.cc
               src/strutil.cc
               src/throttler.cc
               ${PROTO_SRCS} ${PROTO_HDRS}) 

target_link_libraries(flow-disruptor
                      ${PCAP} ${GFLAGS} ${EV} ${PROTOBUF_LIBRARIES})
