cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(example)

#include headers directories
include_directories(
  ../
)

#add source files
set(SOURCE_FILES example.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})