# Project information
cmake_minimum_required(VERSION 3.14)
project(Minote VERSION 0.0
               DESCRIPTION "Modern action puzzle game"
               LANGUAGES C)
set(CMAKE_C_STANDARD 11)

# Module dependencies
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Build the preprocessors
add_executable(Preshade src/tools/preshade.c)
add_executable(Premodel src/tools/premodel.c)
add_subdirectory(lib/msdf-atlasgen)

# Preprocess shaders
include_directories(${PROJECT_BINARY_DIR}/glsl)
set(GLSL_SOURCES
        src/glsl/mino.vert.glsl
        src/glsl/mino.frag.glsl
        src/glsl/scene.vert.glsl
        src/glsl/scene.frag.glsl
        src/glsl/border.vert.glsl
        src/glsl/border.frag.glsl)

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

# Preprocess models
include_directories(${PROJECT_BINARY_DIR}/obj)
set(OBJ_SOURCES src/obj/mino.obj)

foreach(OBJ_PATH ${OBJ_SOURCES})
    get_filename_component(OBJ_FILENAME ${OBJ_PATH} NAME_WLE)
    set(OBJ_OUTPUT ${PROJECT_BINARY_DIR}/obj/${OBJ_FILENAME}.vtx)
    add_custom_command(
            OUTPUT ${OBJ_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/obj/"
            COMMAND Premodel ${PROJECT_SOURCE_DIR}/${OBJ_PATH} ${OBJ_OUTPUT}
            DEPENDS ${OBJ_PATH}
            VERBATIM)
    list(APPEND OBJ_OUTPUTS ${OBJ_OUTPUT})
endforeach(OBJ_PATH)

add_custom_target(Preprocess_models
        DEPENDS ${OBJ_OUTPUTS})

# Preprocess fonts
include_directories(${PROJECT_BINARY_DIR}/ttf)
set(TTF_SOURCES
        assets/ttf/Bitter-Regular.ttf
        assets/ttf/Bitter-RegularItalic.ttf
        assets/ttf/Bitter-Bold.ttf)

foreach(TTF_PATH ${TTF_SOURCES})
    get_filename_component(TTF_FILENAME ${TTF_PATH} NAME_WLE)
    set(TTF_OUTPUT ${PROJECT_BINARY_DIR}/ttf/${TTF_FILENAME})
    add_custom_command(
            OUTPUT ${TTF_OUTPUT}_img.png
            OUTPUT ${TTF_OUTPUT}_desc.c
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/ttf/"
            COMMAND msdf-atlasgen -C 0-1024 -T 512x512 --auto-height 1 -F ${PROJECT_SOURCE_DIR}/${TTF_PATH} -O ${TTF_OUTPUT}
            DEPENDS ${TTF_PATH}
            VERBATIM)
    list(APPEND TTF_OUTPUTS ${TTF_OUTPUT}_img.png ${TTF_OUTPUT}_desc.c)
endforeach(TTF_PATH)

add_custom_target(Preprocess_fonts
        DEPENDS ${TTF_OUTPUTS})

# Build the game
include_directories(src)
include_directories(lib)
set(C_INTERNALLIBS
        lib/glad/glad.c
        lib/pcg/pcg_basic.c)
set(C_SOURCES
        src/fifo.c
        src/gameplay.c
        src/input.c
        src/log.c
        src/logic.c
        src/main.c
        src/mino.c
        src/textrender.c
        src/scenerender.c
        src/minorender.c
        src/borderrender.c
        src/queue.c
        src/render.c
        src/state.c
        src/thread.c
        src/timer.c
        src/util.c
        src/window.c)

add_executable(Minote ${C_SOURCES} ${C_INTERNALLIBS})

add_dependencies(Minote Preprocess_shaders)
add_dependencies(Minote Preprocess_models)
add_dependencies(Minote Preprocess_fonts)
add_dependencies(Preprocess_shaders Preshade)
add_dependencies(Preprocess_models Premodel)
add_dependencies(Preprocess_fonts msdf-atlasgen)

# Link the game
find_package(glfw3 3.3 REQUIRED)
target_link_libraries(Minote glfw)
if(WIN32)
    target_link_libraries(Minote winmm)
endif(WIN32)
target_link_libraries(Minote ${CMAKE_DL_LIBS})
target_link_libraries(Minote Threads::Threads)
target_link_libraries(Minote m)