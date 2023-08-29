cmake_minimum_required(VERSION 3.26)
project(x86-injector)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../InstanceManager)
set(CMAKE_GENERATOR_PLATFORM X86)

add_executable(${PROJECT_NAME} main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" COMPILE_FLAGS "" LINK_FLAG "/MACHINE:X86")

target_link_options(${PROJECT_NAME} PRIVATE "/MACHINE:X86")