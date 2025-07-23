cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(test_actuator)

#include headers directories
include_directories(../)

#add source files
set(SOURCE_FILES test_actuator.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (UNIX)
  target_link_libraries(${PROJECT_NAME} -lpthread)
endif (UNIX)
