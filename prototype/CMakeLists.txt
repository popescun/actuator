# Copyright (c) 2026 Nicolae Popescu. MIT License.
#
# Prototypes of a ulang style struct connection mechanism. Each approach is a separate header with
# its own demo; they do not have to be used together.

cmake_minimum_required(VERSION 4.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(connection_prototypes)

#include headers directories
include_directories(
  ../
  ./include
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(intrinsic_interface_using_macro src/1_intrinsic_interface_using_macro_example.cpp)
add_executable(intrinsic_interface_using_slots src/2_intrinsic_interface_using_slots_example.cpp)
add_executable(intrinsic_interface_using_nttp src/3_intrinsic_interface_using_nttp_example.cpp)
add_executable(intrinsic_interface_using_named src/4_intrinsic_interface_using_named_example.cpp)

# The same connections declared in a class instead of a struct - approach D throughout.
add_executable(class_endpoints src/class_endpoints_example.cpp)
