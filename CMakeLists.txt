# Project information
cmake_minimum_required(VERSION 3.14)
project(Minote VERSION 0.0
               DESCRIPTION "Modern action puzzle game"
               LANGUAGES C)
set(CMAKE_C_STANDARD 11)

# Module dependencies
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_subdirectory(lib/msdfgen)

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

# Preprocess fonts
set(TTF_SOURCES assets/ttf/B612_Mono/B612Mono-Regular.ttf)
foreach(TTF_PATH ${TTF_SOURCES})
    get_filename_component(TTF_FILENAME ${TTF_PATH} NAME_WLE)
    foreach(ITERATOR RANGE 32 126)
        set(TTF_OUTPUT ${PROJECT_BINARY_DIR}/ttf/${TTF_FILENAME}-${ITERATOR}.png)
        add_custom_command(
                OUTPUT ${TTF_OUTPUT}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/ttf/"
                COMMAND msdfgen -font ${PROJECT_SOURCE_DIR}/${TTF_PATH} ${ITERATOR} -size 64 64 -pxrange 12 -scale 1 -translate 4 20 -o ${TTF_OUTPUT}
                DEPENDS ${TTF_PATH}
                VERBATIM)
        list(APPEND TTF_OUTPUTS ${TTF_OUTPUT})
    endforeach(ITERATOR)
endforeach(TTF_PATH)

add_custom_target(Preprocess_fonts
        DEPENDS ${TTF_OUTPUTS})

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
        src/textrender.c
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
add_dependencies(Minote Preprocess_fonts)
add_dependencies(Preprocess_fonts msdfgen)

# Link the game
find_package(glfw3 3.3 REQUIRED)
target_link_libraries(Minote glfw)
if(WIN32)
    target_link_libraries(Minote winmm)
endif(WIN32)
target_link_libraries(Minote ${CMAKE_DL_LIBS})
target_link_libraries(Minote Threads::Threads)
