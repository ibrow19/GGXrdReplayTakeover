cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(PROJECT_NAME GGXrdReplayTakeover)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_GENERATOR_PLATFORM Win32)

project(${PROJECT_NAME} C CXX)

# set the output directory for built objects
# This makes sure that the dynamic library goes into the build directory automatically
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# TODO: check which of these we actually need.
set(IMGUI_SOURCES imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp)
set(IMGUI_BACKEND_SOURCES imgui/backends/imgui_impl_dx9.cpp imgui/backends/imgui_impl_win32.cpp)
set(IMGUI_DIRS imgui imgui/backends/)

# Detours
set(DETOURS_LIB ${PROJECT_SOURCE_DIR}/Detours/lib.X86/detours.lib)
set(DETOURS_INCLUDES Detours/include/)

set(INJECTOR_DIR injector)
file(GLOB_RECURSE INJECTOR_SOURCES ${INJECTOR_DIR}/*.cpp)

set(TEST_DIR load-test)
file(GLOB_RECURSE TEST_SOURCES ${TEST_DIR}/*.cpp)

set(REPLAY_CONTROLLER_LIB_NAME GGXrdReplayController)
set(REPLAY_CONTROLLER_DIR replay-controller)
file(GLOB_RECURSE REPLAY_CONTROLLER_SOURCES ${REPLAY_CONTROLLER_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${INJECTOR_SOURCES})

add_executable(TestLoad ${TEST_SOURCES})

add_library(${REPLAY_CONTROLLER_LIB_NAME} SHARED ${REPLAY_CONTROLLER_SOURCES} ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})
target_link_libraries(${REPLAY_CONTROLLER_LIB_NAME} d3d9.lib ${DETOURS_LIB})
include_directories(${REPLAY_CONTROLLER_LIB_NAME} PRIVATE ${REPLAY_CONTROLLER_DIR} ${IMGUI_DIRS} ${DETOURS_INCLUDES})

set(D3D9_TEST_DIR d3d9-test)
file(GLOB_RECURSE D3D9_TEST_SOURCES ${D3D9_TEST_DIR}/*.cpp)
add_executable(D3D9Test_DebugInfo WIN32 ${D3D9_TEST_SOURCES} ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})

target_link_libraries(D3D9Test_DebugInfo d3d9.lib ${DETOURS_LIB})
include_directories(D3D9Test_DebugInfo PRIVATE ${IMGUI_DIRS} ${DETOURS_INCLUDES})
