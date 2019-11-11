# Project information
cmake_minimum_required(VERSION 3.14)
project(Minote VERSION 0.0
        DESCRIPTION "Modern action puzzle game"
        LANGUAGES C)

# Build settings
set(CMAKE_C_STANDARD 11)
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

# External library dependencies
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
find_library(LIBUNISTRING NAMES libunistring unistring)
if(NOT LIBUNISTRING)
    message(FATAL_ERROR "Library libunistring not found")
endif()
find_package(Iconv REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${Iconv_INCLUDE_DIRS})

# Build the preprocessors
add_executable(Preshade src/tools/preshade.c)
add_executable(Premodel src/tools/premodel.c
        lib/readall/readall.c)

# Preprocess shaders
include_directories(${PROJECT_BINARY_DIR}/glsl)
set(GLSL_SOURCES
        src/glsl/mino.vert.glsl
        src/glsl/mino.frag.glsl
        src/glsl/scene.vert.glsl
        src/glsl/scene.frag.glsl
        src/glsl/border.vert.glsl
        src/glsl/border.frag.glsl
        src/glsl/text.vert.glsl
        src/glsl/text.frag.glsl
        src/glsl/threshold.vert.glsl
        src/glsl/threshold.frag.glsl
        src/glsl/blur.vert.glsl
        src/glsl/blur.frag.glsl
        src/glsl/compose.vert.glsl
        src/glsl/compose.frag.glsl
        src/glsl/particle.vert.glsl
        src/glsl/particle.frag.glsl
        src/glsl/vignette.vert.glsl
        src/glsl/vignette.frag.glsl)

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

# Preprocess models
include_directories(${PROJECT_BINARY_DIR}/obj)
set(OBJ_SOURCES src/obj/mino.obj)

foreach (OBJ_PATH ${OBJ_SOURCES})
    get_filename_component(OBJ_FILENAME ${OBJ_PATH} NAME_WLE)
    set(OBJ_OUTPUT ${PROJECT_BINARY_DIR}/obj/${OBJ_FILENAME}.vtx)
    add_custom_command(
            OUTPUT ${OBJ_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/obj/"
            COMMAND Premodel ${PROJECT_SOURCE_DIR}/${OBJ_PATH} ${OBJ_OUTPUT}
            DEPENDS ${OBJ_PATH}
            VERBATIM)
    list(APPEND OBJ_OUTPUTS ${OBJ_OUTPUT})
endforeach (OBJ_PATH)

add_custom_target(Preprocess_models
        DEPENDS ${OBJ_OUTPUTS})

# Build the game
include_directories(src)
include_directories(lib)
set(C_INTERNALLIBS
        lib/glad/glad.c
        lib/pcg/pcg_basic.c
        lib/readall/readall.c
        lib/parson/parson.c
        lib/AHEasing/easing.c)
set(C_SOURCES
        src/main/main.h src/main/main.c
        src/main/window.h src/main/window.c
        src/main/input.h src/main/input.c
        src/logic/logic.h src/logic/logic.c
        src/logic/gameplay.h src/logic/gameplay.c
        src/logic/pure.h src/logic/pure.c
        src/logic/menu.h src/logic/menu.c
        src/render/render.h src/render/render.c
        src/render/text.h src/render/text.c
        src/render/scene.h src/render/scene.c
        src/render/mino.h src/render/mino.c
        src/render/border.h src/render/border.c
        src/render/post.h src/render/post.c
        src/render/particle.h src/render/particle.c
        src/render/font.h src/render/font.c
        src/render/ease.h src/render/ease.c
        src/global/state.h src/global/state.c
        src/global/settings.h src/global/settings.c
        src/global/effects.h src/global/effects.c
        src/types/fifo.h src/types/fifo.c
        src/types/array.h src/types/array.c
        src/types/mino.h src/types/mino.c
        src/types/game.h
        src/types/menu.h
        src/util/util.h src/util/util.c
        src/util/log.h src/util/log.c
        src/util/thread.h src/util/thread.c
        src/util/timer.h src/util/timer.c)

add_executable(Minote ${C_SOURCES} ${C_INTERNALLIBS})
add_custom_command(TARGET Minote POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/ttf ${PROJECT_BINARY_DIR}/ttf)
add_custom_command(TARGET Minote POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/conf ${PROJECT_BINARY_DIR}/conf)

add_dependencies(Minote Preprocess_shaders)
add_dependencies(Minote Preprocess_models)
add_dependencies(Preprocess_shaders Preshade)
add_dependencies(Preprocess_models Premodel)

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
target_link_libraries(Minote ${LIBUNISTRING})
target_link_libraries(Minote ${Iconv_LIBRARIES})
if (WIN32)
    target_link_libraries(Minote winmm)
endif (WIN32)
target_link_libraries(Minote ${CMAKE_DL_LIBS})
target_link_libraries(Minote Threads::Threads)
target_link_libraries(Minote m)

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