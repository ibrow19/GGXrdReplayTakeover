cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(PROJECT_NAME GGXrdReplayTakeover)
set(CMAKE_BUILD_TYPE Debug)

project(${PROJECT_NAME} C CXX)

# set the output directory for built objects
# This makes sure that the dynamic library goes into the build directory automatically
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# TODO: check which of these we actually need.
set(IMGUI_SOURCES imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp)
set(IMGUI_DIRS imgui imgui/backends)

set(INJECTOR_DIR injector)
file(GLOB_RECURSE INJECTOR_SOURCES ${INJECTOR_DIR}/*.cpp)

set(REPLAY_CONTROLLER_LIB_NAME GGXrdReplayController)
set(REPLAY_CONTROLLER_DIR replay-controller)
file(GLOB_RECURSE REPLAY_CONTROLLER_SOURCES ${REPLAY_CONTROLLER_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${INJECTOR_SOURCES})

add_library(${REPLAY_CONTROLLER_LIB_NAME} SHARED ${REPLAY_CONTROLLER_SOURCES} ${IMGUI_SOURCES})
include_directories(${REPLAY_CONTROLLER_LIB_NAME} PRIVATE ${IMGUI_DIRS})
