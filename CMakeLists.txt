# Project information
cmake_minimum_required(VERSION 3.17)
project(Minote VERSION 0.0
        DESCRIPTION "Modern action puzzle game"
        LANGUAGES C CXX)
include(FetchContent)
cmake_policy(SET CMP0077 NEW)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)
set(CMAKE_CXX_EXTENSIONS OFF)

# Main executable
set(SOURCES
        src/base/concepts.hpp
        src/base/hashmap.hpp
        src/base/svector.hpp
        src/base/version.hpp
        src/base/tween.hpp
        src/base/types.hpp
        src/base/ring.hpp src/base/ring.tpp
        src/base/util.hpp
        src/base/math.hpp
        src/base/time.hpp
        src/base/ease.hpp
        src/base/log.hpp src/base/log.tpp src/base/log.cpp
        src/base/rng.hpp
        src/base/file.hpp src/base/file.cpp
        src/base/id.hpp
        src/sys/keyboard.hpp src/sys/keyboard.cpp
        src/sys/window.hpp src/sys/window.tpp src/sys/window.cpp
        src/sys/glfw.hpp src/sys/glfw.cpp
        src/gfx/engine.hpp src/gfx/engine.cpp
        src/gfx/world.hpp
        src/gfx/imgui.hpp src/gfx/imgui.cpp
        src/gfx/mesh.hpp
        src/gfx/base.hpp
        src/mesh/block.hpp
        src/mesh/scene.hpp
        #src/playstate.hpp src/playstate.cpp
        src/mapper.hpp src/mapper.tpp src/mapper.cpp
        src/config.hpp
        src/game.hpp src/game.cpp
        src/main.hpp src/main.cpp
        src/mino.hpp src/mino.tpp)

# Internal libraries
set(INTERNALLIBS
        lib/static_vector/fixed_capacity_vector
        lib/volk/volk.h lib/volk/volk.c
        lib/stb/stb_image.h lib/stb/stb_image.c)
add_executable(Minote ${SOURCES} ${INTERNALLIBS})

target_include_directories(Minote PRIVATE lib)

add_compile_definitions(VK_NO_PROTOTYPES)
if(WIN32)
    add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
else()
    add_compile_definitions(VK_USE_PLATFORM_XCB_KHR)
endif()

set(VUK_LINK_TO_LOADER OFF CACHE BOOL "")
set(VUK_USE_VULKAN_SDK OFF CACHE BOOL "")
set(VUK_USE_SHADERC OFF CACHE BOOL "")
FetchContent_Declare(vuk
        GIT_REPOSITORY https://github.com/martty/vuk
        GIT_TAG edeb5ab431b25b4c3660936a8c4bcc832d973ffe)
FetchContent_MakeAvailable(vuk)
target_compile_definitions(vuk PUBLIC VUK_CUSTOM_VULKAN_HEADER="volk.h")
target_include_directories(vuk PUBLIC lib/volk)
target_link_libraries(Minote vuk)

FetchContent_Declare(vk-bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG 85aac8d51ce8db80ac254d657a62a7e98a9ad2b4)
FetchContent_MakeAvailable(vk-bootstrap)
target_link_libraries(Minote vk-bootstrap)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(IMGUI_DISABLE_WIN32_FUNCTIONS)
    FetchContent_Declare(imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui
            GIT_TAG 00d570e280533bca9cf6e5652d0f2eb131d032e7)
    FetchContent_MakeAvailable(imgui)
    add_library(imgui
            ${imgui_SOURCE_DIR}/imgui.h ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imconfig.h ${imgui_SOURCE_DIR}/imgui_internal.h
            ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)
    target_include_directories(imgui PRIVATE ${imgui_SOURCE_DIR})
    target_include_directories(Minote PRIVATE ${imgui_SOURCE_DIR})
    target_link_libraries(Minote imgui)
endif()

FetchContent_Declare(pcg
        GIT_REPOSITORY https://github.com/imneme/pcg-c-basic
        GIT_TAG bc39cd76ac3d541e618606bcc6e1e5ba5e5e6aa3)
FetchContent_MakeAvailable(pcg)
add_library(pcg ${pcg_SOURCE_DIR}/pcg_basic.h ${pcg_SOURCE_DIR}/pcg_basic.c)
target_include_directories(Minote PRIVATE ${pcg_SOURCE_DIR})
target_link_libraries(Minote pcg)

if(NOT TARGET robin_hood)
    FetchContent_Declare(robin_hood
            GIT_REPOSITORY https://github.com/martinus/robin-hood-hashing
            GIT_TAG f7aa87e65e6e3bac0dd92df14f192ca882b54d74)
    FetchContent_MakeAvailable(robin_hood)
endif()
target_link_libraries(Minote robin_hood)

# Build settings
target_compile_options(Minote PRIVATE -Wall -Wextra -Wno-missing-field-initializers -fno-rtti)
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
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET Minote PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
target_include_directories(Minote PRIVATE src)

# External libraries
if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
pkg_search_module(GLM REQUIRED glm)
pkg_search_module(FMT REQUIRED fmt)

add_compile_definitions(GLFW_INCLUDE_NONE)
add_compile_definitions(GLM_FORCE_SIZE_T_LENGTH)
add_compile_definitions(GLM_FORCE_UNRESTRICTED_GENTYPE)
add_compile_definitions(GLM_FORCE_LEFT_HANDED)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_include_directories(Minote PRIVATE ${GLFW_INCLUDE_DIRS})
target_include_directories(Minote PRIVATE ${GLM_INCLUDE_DIRS})
target_include_directories(Minote PRIVATE ${FMT_INCLUDE_DIRS})
target_link_libraries(Minote ${GLFW_STATIC_LIBRARIES})
target_link_libraries(Minote ${GLM_STATIC_LIBRARIES})
target_link_libraries(Minote ${FMT_STATIC_LIBRARIES})
if(WIN32)
    target_link_libraries(Minote winmm)
endif()

# Shader compilation
set(SHADER_SOURCES
        src/glsl/swapchainBlit.vert src/glsl/swapchainBlit.frag
        src/glsl/object.vert src/glsl/object.frag
        src/glsl/msmdepth.vert src/glsl/msmdepth.frag
        src/glsl/msmblur.vert src/glsl/msmblur.frag
        src/glsl/msmmoments.vert src/glsl/msmmoments.frag
        src/glsl/bloomThreshold.vert src/glsl/bloomThreshold.frag
        src/glsl/bloomBlur.vert src/glsl/bloomBlur.frag
        src/glsl/imgui.vert src/glsl/imgui.frag)

foreach (SHADER_PATH ${SHADER_SOURCES})
    get_filename_component(SHADER_FILENAME ${SHADER_PATH} NAME)
    set(SHADER_OUTPUT ${PROJECT_BINARY_DIR}/generated/spv/${SHADER_FILENAME}.spv)
    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/generated/spv/"
            COMMAND glslc $<IF:$<CONFIG:Debug>,-O0,-O> -mfmt=num -o ${SHADER_OUTPUT} ${PROJECT_SOURCE_DIR}/${SHADER_PATH}
            DEPENDS ${SHADER_PATH}
            VERBATIM)
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach (SHADER_PATH)

add_custom_target(Compile_shaders DEPENDS ${SHADER_OUTPUTS})
target_include_directories(Minote PRIVATE ${PROJECT_BINARY_DIR}/generated)
add_dependencies(Minote Compile_shaders)
