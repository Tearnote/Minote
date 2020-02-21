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

# Build the preprocessors
add_executable(Preshade src/tools/preshade.c)

# Preprocess shaders
include_directories(${PROJECT_BINARY_DIR}/glsl)
set(GLSL_SOURCES
        src/glsl/flat.vert.glsl
        src/glsl/flat.frag.glsl
        src/glsl/phong.vert.glsl
        src/glsl/phong.frag.glsl)

foreach (GLSL_PATH ${GLSL_SOURCES})
    get_filename_component(GLSL_FILENAME ${GLSL_PATH} NAME_WLE)
    set(GLSL_OUTPUT ${PROJECT_BINARY_DIR}/glsl/${GLSL_FILENAME})
    add_custom_command(
            OUTPUT ${GLSL_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/glsl/"
            COMMAND Preshade ${PROJECT_SOURCE_DIR}/${GLSL_PATH} ${GLSL_OUTPUT}
            DEPENDS ${GLSL_PATH}
            VERBATIM)
    list(APPEND GLSL_OUTPUTS ${GLSL_OUTPUT})
endforeach (GLSL_PATH)

add_custom_target(Preprocess_shaders
        DEPENDS ${GLSL_OUTPUTS})

# Build the game
include_directories(lib)
set(C_INTERNALLIBS
        lib/glad/glad.h lib/glad/glad.c lib/glad/KHR/khrplatform.h
        lib/linmath/linmath.h
        lib/pcg/pcg_basic.h lib/pcg/pcg_basic.c
        lib/aheasing/easing.h lib/aheasing/easing.c)

set(C_SOURCES
        src/main.h src/main.c
        src/log.h src/log.c
        src/util.h src/util.c
        src/window.h src/window.c
        src/basetypes.h src/basetypes.c
        src/queue.h src/queue.c
        src/game.h src/game.c
        src/thread.h src/thread.c
        src/system.h src/system.c
        src/time.h src/time.c
        src/renderer.h src/renderer.c
        src/shader.h src/shader.c
        src/darray.h src/darray.c
        src/play.h src/play.c
        src/mino.h src/mino.c
        src/mapper.h src/mapper.c
        src/pure.h src/pure.c
        src/puretables.h)

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

add_dependencies(Minote Preprocess_shaders)
add_dependencies(Preprocess_shaders Preshade)

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
if(WIN32)
    target_link_libraries(Minote winmm)
endif()
target_link_libraries(Minote ${CMAKE_DL_LIBS})
target_link_libraries(Minote Threads::Threads)
target_link_libraries(Minote m)
