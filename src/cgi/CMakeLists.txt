cmake_minimum_required(VERSION 3.10)

project(cgi)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/cgi-bin)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(insert.cgi ../../include/base64.h insert.cgi.cpp)
target_link_libraries(insert.cgi fmt::fmt)

add_executable(view.cgi ../../include/base64.h view.cgi.cpp)
target_link_libraries(view.cgi fmt::fmt)

add_executable(viewstatic.cgi ../../include/base64.h viewstatic.cgi.cpp)
target_link_libraries(viewstatic.cgi fmt::fmt)

