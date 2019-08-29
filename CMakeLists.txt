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

# Preprocess shaders
include_directories(${PROJECT_BINARY_DIR}/glsl)
set(GLSL_SOURCES
        src/glsl/mino.vert.glsl
        src/glsl/mino.frag.glsl
        src/glsl/scene.vert.glsl
        src/glsl/scene.frag.glsl
        src/glsl/border.vert.glsl
        src/glsl/border.frag.glsl)
set(GLSL_INTERNALLIBS
        lib/msdfgl/msdfgl_msdf.vert.glsl
        lib/msdfgl/msdfgl_msdf.frag.glsl
        lib/msdfgl/msdfgl_font.geom.glsl
        lib/msdfgl/msdfgl_font.vert.glsl
        lib/msdfgl/msdfgl_font.frag.glsl)
list(APPEND GLSL_SOURCES ${GLSL_INTERNALLIBS})

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

# Build the game
include_directories(src)
include_directories(lib)
set(C_INTERNALLIBS
        lib/glad/glad.c
        lib/pcg/pcg_basic.c
        lib/msdfgl/msdfgl.c
        lib/msdfgl/msdfgl_map.c
        lib/msdfgl/msdfgl_serializer.c)
set(C_SOURCES
        src/fifo.c
        src/gameplay.c
        src/input.c
        src/log.c
        src/logic.c
        src/main.c
        src/mino.c
        src/scenerender.c
        src/minorender.c
        src/borderrender.c
        src/queue.c
        src/render.c
        src/state.c
        src/thread.c
        src/timer.c
        src/util.c
        src/window.c src/textrender.h src/textrender.c)

add_executable(Minote ${C_SOURCES} ${C_INTERNALLIBS})

add_dependencies(Minote Preprocess_shaders)
add_dependencies(Minote Preprocess_models)
add_dependencies(Preprocess_shaders Preshade)
add_dependencies(Preprocess_models Premodel)

# Link internal libraries
find_package(Freetype)
include_directories(${FREETYPE_INCLUDE_DIRS})
target_link_libraries(Minote ${FREETYPE_LIBRARIES})

# Link the game
find_package(glfw3 3.3 REQUIRED)
target_link_libraries(Minote glfw)
if(WIN32)
    target_link_libraries(Minote winmm)
endif(WIN32)
target_link_libraries(Minote ${CMAKE_DL_LIBS})
target_link_libraries(Minote Threads::Threads)
target_link_libraries(Minote m)
