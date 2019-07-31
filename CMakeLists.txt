# Project information
cmake_minimum_required(VERSION 3.14)
project(Minote VERSION 0.0
               DESCRIPTION "Modern action puzzle game"
               LANGUAGES C)
set(CMAKE_C_STANDARD 11)

# Build the shader preprocessor
add_executable(Preshade tools/preshade.c)

# Preprocess shaders
include_directories(${PROJECT_BINARY_DIR}/glsl)
set(GLSL_SOURCES src/glsl/mino.vert.glsl
        src/glsl/mino.frag.glsl)

foreach(GLSL_PATH ${GLSL_SOURCES})
    get_filename_component(GLSL_FILENAME ${GLSL_PATH} NAME_WLE)
    set(GLSL_OUTPUT ${PROJECT_BINARY_DIR}/glsl/${GLSL_FILENAME})
    add_custom_command(
            OUTPUT ${GLSL_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/glsl/"
            COMMAND Preshade ${PROJECT_SOURCE_DIR}/${GLSL_PATH} ${GLSL_OUTPUT}
            DEPENDS ${GLSL_PATH}
            VERBATIM)
    list(APPEND GLSL_OUTPUTS ${GLSL_OUTPUT})
endforeach(GLSL_PATH)

add_custom_target(Preprocess_shaders
        DEPENDS ${GLSL_OUTPUTS})

# Build the game
include_directories(src)
set(C_SOURCES
        src/glad/glad.c
        src/pcg/pcg_basic.c
        src/fifo.c
        src/gameplay.c
        src/input.c
        src/log.c
        src/logic.c
        src/main.c
        src/mino.c
        src/minorender.c
        src/queue.c
        src/render.c
        src/state.c
        src/thread.c
        src/timer.c
        src/util.c
        src/window.c)

add_executable(Minote ${C_SOURCES})

add_dependencies(Minote Preprocess_shaders)
add_dependencies(Preprocess_shaders Preshade)

# Link the game
find_package(glfw3 3.3 REQUIRED)
target_link_libraries(Minote glfw)
target_link_libraries(Minote winmm)