cmake_minimum_required(VERSION 3.15)
project(cube-demo VERSION 1.0 LANGUAGES C)

# Defaults
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build with static libs
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glfw)

add_library(glad STATIC ${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/third_party/glad/include)

add_library(nuklear INTERFACE)
target_include_directories(nuklear
  INTERFACE 
    ${CMAKE_SOURCE_DIR}/third_party/Nuklear
    ${CMAKE_SOURCE_DIR}/third_party/Nuklear/demo/glfw_opengl3
)

# Executable and deps
add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.c)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    glfw
    glad
    nuklear
)

# Linking and compile options
if (WIN32)
  if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /MT /wd4116)
  else()
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static)
  endif()

  # Windows OpenGL
  target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)

elseif (APPLE)
  # force clang for ObjC
  set(CMAKE_C_COMPILER /usr/bin/clang CACHE STRING "" FORCE)
  set(CMAKE_CXX_COMPILER /usr/bin/clang++ CACHE STRING "" FORCE)

  # ObjC flags for GLFW
  target_compile_options(glfw PRIVATE
    $<$<COMPILE_LANGUAGE:OBJC>:-fblocks>
  )

  # Cocoa + OpenGL framework
  find_library(COCOA Cocoa REQUIRED)
  find_library(OPENGL OpenGL REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${COCOA} ${OPENGL})

  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/macos/Info.plist"
  )

else() # Linux
  find_package(OpenGL REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
endif()