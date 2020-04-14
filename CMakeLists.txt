# Project definition
cmake_minimum_required(VERSION 3.10)
project(psxemu)

# Source files
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source/core")
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source/imgui")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")

# Executable definition and properties
add_subdirectory("${SRC_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${SRC_DIR}")
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# IMGUI
add_subdirectory("${IMGUI_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${IMGUI_DIR}")

# GLAD
set(GLAD_DIR "${LIB_DIR}/glad")
add_subdirectory("${GLAD_DIR}/cmake" glad_cmake)
glad_add_library(glad REPRODUCIBLE API gl:core=3.3)

# GLFW
set(GLFW_DIR "${LIB_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${GLFW_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLFW_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_NONE")

# GLM
set(GLM_DIR "${LIB_DIR}/glm")
add_subdirectory("${GLM_DIR}" EXCLUDE_FROM_ALL)