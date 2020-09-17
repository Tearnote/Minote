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

# External library dependencies
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
pkg_search_module(FREETYPE REQUIRED freetype2)
pkg_search_module(HARFBUZZ REQUIRED harfbuzz)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Build the preprocessors
add_executable(Preshade src/tools/preshade.c)
target_include_directories(Preshade PRIVATE lib)

add_subdirectory(lib/msdf-atlas-gen)

# Preprocess shaders
set(GLSL_SOURCES
        src/glsl/flat.vert.glsl src/glsl/flat.frag.glsl
        src/glsl/phong.vert.glsl src/glsl/phong.frag.glsl
        src/glsl/smaaSeparate.vert.glsl src/glsl/smaaSeparate.frag.glsl
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
            COMMAND msdf-atlas-gen -font ${FONT_PATH} -pxrange 4 -charset "${FONT_DIR}/${FONT_NAME}.txt" -imageout ${FONT_OUTPUT}.png -csv ${FONT_OUTPUT}.csv -json ${FONT_OUTPUT}.json
            COMMAND ${CMAKE_COMMAND} -E copy ${FONT_PATH} "${PROJECT_BINARY_DIR}/fonts/${FONT_FILENAME}"
            DEPENDS ${FONT_PATH}
            VERBATIM)
    list(APPEND FONT_OUTPUTS ${FONT_OUTPUT}.png ${FONT_OUTPUT}.csv)
endforeach (FONT_PATH)

add_custom_target(Preprocess_fonts DEPENDS ${FONT_OUTPUTS})
add_dependencies(Preprocess_fonts msdf-atlas-gen)

# Build the game
set(C_INTERNALLIBS
        lib/glad/glad.h lib/glad/glad.c lib/glad/KHR/khrplatform.h
        lib/linmath/linmath.h
        lib/pcg/pcg_basic.h lib/pcg/pcg_basic.c
        lib/aheasing/easing.h lib/aheasing/easing.c
        lib/smaa/AreaTex.h lib/smaa/SearchTex.h
        lib/nuklear/nuklear.h
        lib/cephes/protos.h lib/cephes/mconf.h
        lib/cephes/const.c lib/cephes/polevl.c lib/cephes/fresnl.c
        lib/stb/stb_image.h lib/stb/stb_image.c)

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
        src/darray.h src/darray.c
        src/play.h src/play.c
        src/mino.h src/mino.c
        src/mapper.h src/mapper.c
        src/mrs.h src/mrs.c
        src/mrsdef.h src/mrsdef.c
        src/mrsdraw.h src/mrsdraw.c
        src/aa.h src/aa.c
        src/opengl.h src/opengl.c
        src/bloom.h src/bloom.c
        src/model.h src/model.c
        src/world.h src/world.c
        src/tween.h src/tween.c
        src/particles.h src/particles.c
        src/debug.h src/debug.c
        src/fontlist.h
        src/font.h src/font.c src/text.c src/text.h)

add_executable(Minote ${C_SOURCES} ${C_INTERNALLIBS})
target_compile_options(Minote PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -pedantic-errors>)

add_dependencies(Minote Preprocess_shaders)
add_dependencies(Minote Preprocess_fonts)

target_include_directories(Minote PRIVATE ${PROJECT_BINARY_DIR}/glsl)
target_include_directories(Minote PRIVATE lib)
target_include_directories(Minote PRIVATE ${GLFW_INCLUDE_DIRS})
target_include_directories(Minote PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
target_include_directories(Minote PRIVATE ${FREETYPE_INCLUDE_DIRS})

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

# https://github.com/google/brotli/issues/795
list(TRANSFORM HARFBUZZ_STATIC_LIBRARIES REPLACE "brotlidec" "brotlidec-static")
list(TRANSFORM FREETYPE_STATIC_LIBRARIES REPLACE "brotlidec" "brotlidec-static")
list(TRANSFORM HARFBUZZ_STATIC_LIBRARIES REPLACE "brotlicommon" "brotlicommon-static")
list(TRANSFORM FREETYPE_STATIC_LIBRARIES REPLACE "brotlicommon" "brotlicommon-static")
target_link_libraries(Minote ${HARFBUZZ_STATIC_LIBRARIES})
target_link_libraries(Minote ${FREETYPE_STATIC_LIBRARIES})
if(WIN32)
    target_link_libraries(Minote stdc++) # libgraphite2 bug https://github.com/msys2/MINGW-packages/issues/1877
    target_link_libraries(Minote winmm)
endif()
target_link_libraries(Minote ${CMAKE_DL_LIBS})
target_link_libraries(Minote Threads::Threads)
target_link_libraries(Minote m)
