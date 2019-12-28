# Project information
cmake_minimum_required(VERSION 3.15)
project(Minote VERSION 0.0
        DESCRIPTION "Modern action puzzle game"
        LANGUAGES C)

# Build settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# External library dependencies
find_package(glfw3 CONFIG REQUIRED)

# Build the game
include_directories(src)
include_directories(lib)
set(C_INTERNALLIBS
        lib/glad/glad.c)
set(C_SOURCES
        src/main.h src/main.c
        src/log.h src/log.c
        src/util.h src/util.c
        src/window.h src/window.c
        src/point.h
        src/queue.h src/queue.c)

add_executable(Minote ${C_SOURCES} ${C_INTERNALLIBS})
add_custom_command(TARGET Minote POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/ttf ${PROJECT_BINARY_DIR}/ttf)
add_custom_command(TARGET Minote POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/conf ${PROJECT_BINARY_DIR}/conf)
target_compile_options(Minote PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -pedantic-errors>
        $<$<CXX_COMPILER_ID:MSVC>:
        /W4>)

# Link the game
target_link_libraries(Minote PRIVATE glfw ${CMAKE_DL_LIBS})
