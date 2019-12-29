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
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s -ffunction-sections -Wl,--gc-sections")
    if(WIN32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mwindows")
    endif()
endif()

# External library settings
if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# External library settings
if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# External library dependencies
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})

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
        src/queue.h src/queue.c
        src/game.h src/game.c
        src/thread.h src/thread.c)

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

# Linker settings
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)
    if(result)
        set_property(TARGET Minote PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "IPO is not supported: ${output}")
    endif()
endif()

# Link the game
target_link_libraries(Minote ${GLFW_STATIC_LIBRARIES})
target_link_libraries(Minote ${CMAKE_DL_LIBS})
target_link_libraries(Minote Threads::Threads)
target_link_libraries(Minote m)
