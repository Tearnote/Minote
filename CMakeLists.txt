# Project information
cmake_minimum_required(VERSION 3.15)
project(Minote VERSION 0.0
        DESCRIPTION "Modern action puzzle game"
        LANGUAGES C CXX)

# Build settings
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s -ffunction-sections -Wl,--gc-sections")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -ffunction-sections -Wl,--gc-sections")
    if(WIN32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mwindows")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
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
pkg_search_module(FREETYPE REQUIRED freetype2)
pkg_search_module(HARFBUZZ REQUIRED harfbuzz)
pkg_search_module(GLM REQUIRED glm)
pkg_search_module(FMT REQUIRED fmt)

add_compile_definitions(GLFW_INCLUDE_NONE)
#add_compile_definitions(GLM_FORCE_ALIGNED_GENTYPES)
#add_compile_definitions(GLM_FORCE_DEFAULT_ALIGNED_GENTYPES)
add_compile_definitions(GLM_FORCE_SIZE_T_LENGTH)
add_compile_definitions(GLM_FORCE_UNRESTRICTED_GENTYPE)

# Build the preprocessors
add_executable(Preshade src/tools/preshade.cpp)
target_include_directories(Preshade PRIVATE lib)

add_subdirectory(lib/msdf-atlas-gen)

# Preprocess shaders
set(GLSL_SOURCES
        src/glsl/flat.vert.glsl src/glsl/flat.frag.glsl
        src/glsl/phong.vert.glsl src/glsl/phong.frag.glsl
        src/glsl/smaaEdge.vert.glsl src/glsl/smaaEdge.frag.glsl
        src/glsl/smaaBlend.vert.glsl src/glsl/smaaBlend.frag.glsl
        src/glsl/smaaNeighbor.vert.glsl src/glsl/smaaNeighbor.frag.glsl
        src/glsl/blit.vert.glsl src/glsl/blit.frag.glsl
        src/glsl/delinearize.vert.glsl src/glsl/delinearize.frag.glsl
        src/glsl/threshold.vert.glsl src/glsl/threshold.frag.glsl
        src/glsl/boxBlur.vert.glsl src/glsl/boxBlur.frag.glsl
        src/glsl/nuklear.vert.glsl src/glsl/nuklear.frag.glsl
        src/glsl/msdf.vert.glsl src/glsl/msdf.frag.glsl)

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

add_custom_target(Preprocess_shaders DEPENDS ${GLSL_OUTPUTS})
add_dependencies(Preprocess_shaders Preshade)

#Preprocess fonts
set(FONT_SOURCES
        fonts/Jost-500-Medium.otf)

foreach (FONT_PATH ${FONT_SOURCES})
    get_filename_component(FONT_DIR ${FONT_PATH} DIRECTORY)
    get_filename_component(FONT_FILENAME ${FONT_PATH} NAME)
    get_filename_component(FONT_NAME ${FONT_PATH} NAME_WLE)
    set(FONT_OUTPUT ${PROJECT_BINARY_DIR}/fonts/${FONT_NAME})
    add_custom_command(
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT ${FONT_OUTPUT}.png ${FONT_OUTPUT}.csv
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/fonts/"
            COMMAND msdf-atlas-gen -font ${FONT_PATH} -pxrange 4 -charset "${FONT_DIR}/${FONT_NAME}.txt" -imageout ${FONT_OUTPUT}.png -csv ${FONT_OUTPUT}.csv
            COMMAND ${CMAKE_COMMAND} -E copy ${FONT_PATH} "${PROJECT_BINARY_DIR}/fonts/${FONT_FILENAME}"
            DEPENDS ${FONT_PATH}
            VERBATIM)
    list(APPEND FONT_OUTPUTS ${FONT_OUTPUT}.png ${FONT_OUTPUT}.csv)
endforeach (FONT_PATH)

add_custom_target(Preprocess_fonts DEPENDS ${FONT_OUTPUTS})
add_dependencies(Preprocess_fonts msdf-atlas-gen)

# Build the game
set(INTERNALLIBS
        lib/robin-hood-hashing/robin_hood.h
        lib/scope_guard/scope_guard.hpp
        lib/xassert/xassert.h lib/xassert/xassert.c
        lib/nuklear/nuklear.h
        lib/cephes/const.c lib/cephes/polevl.c lib/cephes/fresnl.c
        lib/cephes/protos.h lib/cephes/mconf.h
        lib/itlib/static_vector.hpp
        lib/smaa/AreaTex.h lib/smaa/SearchTex.h
        lib/glad/glad.h
        lib/pcg/pcg_basic.h lib/pcg/pcg_basic.c
        lib/stb/stb_image.h lib/stb/stb_image.c)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND INTERNALLIBS lib/glad/release/glad.h lib/glad/release/glad.c
            lib/glad/release/khrplatform.h)
else()
    list(APPEND INTERNALLIBS lib/glad/debug/glad.h lib/glad/debug/glad.c
            lib/glad/debug/khrplatform.h)
endif()

add_compile_definitions(NK_UINT_DRAW_INDEX)

set(SOURCES
        src/main.hpp src/main.cpp
        src/base/hashmap.hpp
        src/base/concept.hpp
        src/base/math_io.hpp
        src/base/time_io.hpp
        src/base/thread.hpp src/base/thread.cpp
        src/base/string.hpp
        src/base/tween.hpp
        src/base/array.hpp
        src/base/ring.hpp src/base/ring.tpp
        src/base/util.hpp
        src/base/ease.hpp
        src/base/math.hpp
        src/base/time.hpp
        src/base/log.hpp src/base/log.cpp
        src/base/rng.hpp
        src/base/io.hpp src/base/io.cpp
        src/sys/opengl/framebuffer.hpp src/sys/opengl/framebuffer.tpp src/sys/opengl/framebuffer.cpp
        src/sys/opengl/vertexarray.hpp src/sys/opengl/vertexarray.tpp src/sys/opengl/vertexarray.cpp
        src/sys/opengl/texture.hpp src/sys/opengl/texture.tpp
        src/sys/opengl/shader.hpp src/sys/opengl/shader.tpp src/sys/opengl/shader.cpp
        src/sys/opengl/buffer.hpp src/sys/opengl/buffer.tpp
        src/sys/opengl/state.hpp src/sys/opengl/state.cpp
        src/sys/opengl/draw.hpp src/sys/opengl/draw.tpp src/sys/opengl/draw.cpp
        src/sys/opengl/base.hpp src/sys/opengl/base.cpp
        src/sys/keyboard.hpp src/sys/keyboard.cpp
        src/sys/window.hpp src/sys/window.cpp
        src/sys/glfw.hpp src/sys/glfw.cpp
        src/engine/mapper.hpp src/engine/mapper.cpp
        src/engine/model.hpp src/engine/model.cpp
        src/engine/frame.hpp src/engine/frame.cpp
        src/engine/scene.hpp src/engine/scene.cpp
        src/engine/font.hpp src/engine/font.cpp
        src/engine/engine.hpp
        src/store/shaders.hpp src/store/shaders.cpp
        src/store/models.hpp src/store/models.cpp
        src/store/fonts.hpp src/store/fonts.cpp
        src/particles.hpp src/particles.cpp
        src/mrsdraw.hpp src/mrsdraw.cpp
        src/mrsdef.hpp src/mrsdef.cpp
        src/bloom.hpp src/bloom.cpp
        src/debug.hpp src/debug.cpp
        src/text.hpp src/text.cpp
        src/game.hpp src/game.cpp
        src/play.hpp src/play.cpp
        src/mino.hpp src/mino.cpp
        src/mrs.hpp src/mrs.cpp)

add_executable(Minote ${SOURCES} ${INTERNALLIBS})
target_compile_options(Minote PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -fno-rtti>)

add_dependencies(Minote Preprocess_shaders)
add_dependencies(Minote Preprocess_fonts)

target_include_directories(Minote PRIVATE src)
target_include_directories(Minote PRIVATE lib)
target_include_directories(Minote PRIVATE ${PROJECT_BINARY_DIR}/glsl)
target_include_directories(Minote PRIVATE ${GLFW_INCLUDE_DIRS})
target_include_directories(Minote PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
target_include_directories(Minote PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_include_directories(Minote PRIVATE ${GLM_INCLUDE_DIRS})
target_include_directories(Minote PRIVATE ${FMT_INCLUDE_DIRS})

# Linker settings
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET Minote PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Link the game
target_link_libraries(Minote ${GLFW_STATIC_LIBRARIES})
target_link_libraries(Minote ${GLM_STATIC_LIBRARIES})
target_link_libraries(Minote ${FMT_STATIC_LIBRARIES})
if(WIN32)
    target_link_libraries(Minote winmm)
endif()

# https://github.com/google/brotli/issues/795
list(TRANSFORM HARFBUZZ_STATIC_LIBRARIES REPLACE "brotlidec" "brotlidec-static")
list(TRANSFORM FREETYPE_STATIC_LIBRARIES REPLACE "brotlidec" "brotlidec-static")
list(TRANSFORM HARFBUZZ_STATIC_LIBRARIES REPLACE "brotlicommon" "brotlicommon-static")
list(TRANSFORM FREETYPE_STATIC_LIBRARIES REPLACE "brotlicommon" "brotlicommon-static")
target_link_libraries(Minote ${HARFBUZZ_STATIC_LIBRARIES})
target_link_libraries(Minote ${FREETYPE_STATIC_LIBRARIES})
