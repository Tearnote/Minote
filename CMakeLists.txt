# Project information
cmake_minimum_required(VERSION 3.15)
project(Minote VERSION 0.0
        DESCRIPTION "Modern action puzzle game"
        LANGUAGES C CXX)

# Build settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -ffunction-sections -Wl,--gc-sections")
    if(WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
    endif()
endif()

# Internal library settings
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(GSL_UNENFORCED_ON_CONTRACT_VIOLATION)
else()
    add_compile_definitions(GSL_THROW_ON_CONTRACT_VIOLATION)
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
include_directories(${GLFW_INCLUDE_DIRS})

# Build the game
include_directories(src)
include_directories(lib)
set(CXX_INTERNALLIBS
        lib/glad/glad.c
        lib/fmt/format.cc)
set(CXX_SOURCES
        src/main.cpp
        src/log.cpp src/log.h src/log.tcc
        src/system.cpp src/system.h
        src/window.cpp src/window.h
        src/point.h
        src/timer.h
        src/game.cpp src/game.h
        src/renderer.cpp src/renderer.h)

add_executable(Minote ${CXX_SOURCES} ${CXX_INTERNALLIBS})
add_custom_command(TARGET Minote POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/ttf ${PROJECT_BINARY_DIR}/ttf)
add_custom_command(TARGET Minote POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/conf ${PROJECT_BINARY_DIR}/conf)

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

# Package the game
install(TARGETS Minote
        RUNTIME DESTINATION .)
install(DIRECTORY ${PROJECT_BINARY_DIR}/ttf DESTINATION .
        FILES_MATCHING PATTERN "*.png")
install(DIRECTORY ${PROJECT_BINARY_DIR}/ttf DESTINATION .
        FILES_MATCHING PATTERN "*.json")
install(DIRECTORY ${PROJECT_BINARY_DIR}/conf DESTINATION .)
set(CPACK_GENERATOR "ZIP")
include(CPack)