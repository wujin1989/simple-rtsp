cmake_minimum_required(VERSION 3.20)

project(simple-rtsp LANGUAGES C)


if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "[examples compile]: current os is 64bit!")
else()
	message(FATAL_ERROR "[examples compile]: 32bit os isn't supported!")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#iso c standard, support c11 and later
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD 11)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include_directories(include)
include_directories(cdk/include)
link_directories(cdk/Debug)
	
set(RTSP_CLIENT_HDRS
    include/rtsp-client/rtsp-client.h
    include/rtsp-common/rtsp-common.h
    include/rtsp-common/sdp.h
    include/rtsp-common/rtp.h
    include/rtsp-common/nalu.h
    include/rtsp-common/hevc.h)

set(RTSP_SERVER_HDRS
    include/rtsp-server/rtsp-server.h
    include/rtsp-common/rtsp-common.h
    include/rtsp-common/sdp.h
    include/rtsp-common/rtp.h
    include/rtsp-common/nalu.h
    include/rtsp-common/hevc.h)

set(RTSP_CLIENT_SRCS 
	src/rtsp-client/main.c
    src/rtsp-client/rtsp-client.c
    src/rtsp-common/rtsp-common.c
    src/rtsp-common/sdp.c
    src/rtsp-common/rtp.c
    src/rtsp-common/nalu.c
    src/rtsp-common/hevc.c)

set(RTSP_SERVER_SRCS 
	src/rtsp-server/main.c
    src/rtsp-server/rtsp-server.c
    src/rtsp-common/rtsp-common.c
    src/rtsp-common/sdp.c
    src/rtsp-common/rtp.c
    src/rtsp-common/nalu.c
    src/rtsp-common/hevc.c)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${RTSP_CLIENT_HDRS} ${RTSP_SERVER_HDRS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${RTSP_CLIENT_SRCS} ${RTSP_SERVER_SRCS})

add_executable(rtsp-server ${RTSP_SERVER_SRCS} ${RTSP_SERVER_HDRS})
target_link_libraries(rtsp-server PUBLIC cdk Threads::Threads)

add_executable(rtsp-client ${RTSP_CLIENT_SRCS} ${RTSP_CLIENT_HDRS})
target_link_libraries(rtsp-client PUBLIC cdk Threads::Threads)

