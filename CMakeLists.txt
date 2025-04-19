cmake_minimum_required(VERSION 3.15)
project(3d-cube-demo VERSION 1.0 LANGUAGES C)

# Project Setup
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Directory Path Setup
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party CACHE PATH "Path to third party libraries")
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src CACHE PATH "Path to source files")
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include CACHE PATH "Public headers")

# Pull GLFW
add_subdirectory(${THIRD_PARTY_DIR}/glfw)

# GLAD loader
add_library(glad STATIC
  ${THIRD_PARTY_DIR}/glad/src/glad.c
)

target_include_directories(glad PUBLIC
  ${THIRD_PARTY_DIR}/glad/include
)

# Nuklear Interface
add_library(nuklear INTERFACE)
target_include_directories(nuklear
    INTERFACE 
    ${THIRD_PARTY_DIR}/Nuklear
    ${THIRD_PARTY_DIR}/Nuklear/demo/glfw_opengl3
)

# Paths for executables
add_executable(${PROJECT_NAME}
    ${SRC_DIR}/main.c
)

# Suppress warnings
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4116)
endif()

# My headers
target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${PROJECT_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glfw
        glad
        nuklear
        # System GL
        $<$<PLATFORM_ID:Windows>:opengl32>
        $<$<PLATFORM_ID:Linux>:OpenGL::GL>
)

# Link correct libs based on OS
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
elseif(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(OpenGL_FRAMEWORK OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_FRAMEWORK}
        ${OpenGL_FRAMEWORK}
    )
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
endif()