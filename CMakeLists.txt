cmake_minimum_required(VERSION 3.6)
project(server_c)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(/opt/microhttpd/include /opt/zlib/include /home/james/school/esw/week4/liburcu/include)
link_directories(/opt/microhttpd/lib /opt/zlib/lib /home/james/school/esw/week4/liburcu/lib)

set(SOURCE_FILES main.cpp)
add_executable(c_server ${SOURCE_FILES})
target_link_libraries(c_server microhttpd z pthread urcu rt)
