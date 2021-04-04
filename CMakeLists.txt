# Project information

cmake_minimum_required(VERSION 3.17)
project(Minote VERSION 0.0
        DESCRIPTION "Modern action puzzle game"
        LANGUAGES C CXX)
cmake_policy(SET CMP0077 NEW)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build settings

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

if(WIN32)
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# Main executable

add_executable(Minote
        src/base/concepts.hpp
        src/base/hashmap.hpp
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
target_compile_options(Minote PRIVATE -Wall -Wextra -Wno-missing-field-initializers -fno-rtti)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET Minote PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
target_include_directories(Minote PRIVATE src)
if(WIN32)
    target_link_libraries(Minote PRIVATE winmm)
endif()

# Libraries

include(FetchContent)

target_compile_definitions(Minote PRIVATE VK_NO_PROTOTYPES)
if(WIN32)
    target_compile_definitions(Minote PRIVATE VK_USE_PLATFORM_WIN32_KHR)
else()
    target_compile_definitions(Minote PRIVATE VK_USE_PLATFORM_XCB_KHR)
endif()

FetchContent_Declare(volk
        GIT_REPOSITORY https://github.com/zeux/volk
        GIT_TAG 66169849935bd85f329c513dd01c28b0a02bfe33)
FetchContent_MakeAvailable(volk)
target_link_libraries(Minote PRIVATE volk)

set(VUK_LINK_TO_LOADER OFF CACHE BOOL "")
set(VUK_USE_VULKAN_SDK OFF CACHE BOOL "")
set(VUK_USE_SHADERC OFF CACHE BOOL "")
FetchContent_Declare(vuk
        GIT_REPOSITORY https://github.com/martty/vuk
        GIT_TAG edeb5ab431b25b4c3660936a8c4bcc832d973ffe)
FetchContent_MakeAvailable(vuk)
target_compile_definitions(vuk PRIVATE VK_NO_PROTOTYPES)
target_compile_definitions(vuk PRIVATE VUK_CUSTOM_VULKAN_HEADER="volk.h")
target_link_libraries(vuk PRIVATE volk)
target_link_libraries(Minote PRIVATE vuk)

FetchContent_Declare(vk-bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG 85aac8d51ce8db80ac254d657a62a7e98a9ad2b4)
FetchContent_MakeAvailable(vk-bootstrap)
target_link_libraries(Minote PRIVATE vk-bootstrap)

FetchContent_Declare(pcg
        GIT_REPOSITORY https://github.com/imneme/pcg-c-basic
        GIT_TAG bc39cd76ac3d541e618606bcc6e1e5ba5e5e6aa3)
FetchContent_MakeAvailable(pcg)
add_library(pcg ${pcg_SOURCE_DIR}/pcg_basic.h ${pcg_SOURCE_DIR}/pcg_basic.c)
target_include_directories(Minote PRIVATE ${pcg_SOURCE_DIR})
target_link_libraries(Minote PRIVATE pcg)

if(NOT TARGET robin_hood)
    FetchContent_Declare(robin_hood
            GIT_REPOSITORY https://github.com/martinus/robin-hood-hashing
            GIT_TAG f7aa87e65e6e3bac0dd92df14f192ca882b54d74)
    FetchContent_MakeAvailable(robin_hood)
endif()
target_link_libraries(Minote PRIVATE robin_hood)

FetchContent_Declare(fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG 95da4847274c42c110fd5bf1bf61d3f94be4a08e)
FetchContent_MakeAvailable(fmt)
target_link_libraries(Minote PRIVATE fmt)

target_compile_definitions(Minote PRIVATE GLM_FORCE_SIZE_T_LENGTH)
target_compile_definitions(Minote PRIVATE GLM_FORCE_UNRESTRICTED_GENTYPE)
target_compile_definitions(Minote PRIVATE GLM_FORCE_LEFT_HANDED)
target_compile_definitions(Minote PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)
FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG ace16e47780dcef815294715237f51e9129b6eb3)
FetchContent_MakeAvailable(glm)
target_include_directories(Minote PRIVATE ${glm_SOURCE_DIR}) # The target isn't working for some reason

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
target_compile_definitions(Minote PRIVATE GLFW_INCLUDE_NONE)
FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG 901d30b973c536f26b74bca1134c5892d54b251d) # 3.3.3
FetchContent_MakeAvailable(glfw)
target_link_libraries(Minote PRIVATE glfw)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
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
    target_compile_definitions(imgui PRIVATE IMGUI_DISABLE_WIN32_FUNCTIONS)
    target_include_directories(imgui PRIVATE ${imgui_SOURCE_DIR})
    target_include_directories(Minote PRIVATE ${imgui_SOURCE_DIR})
    target_link_libraries(imgui PRIVATE glfw)
    target_link_libraries(Minote PRIVATE imgui)
endif()

set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "")
FetchContent_Declare(spirv-headers
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
        GIT_TAG f88a1f98fa7a44ccfcf33d810c72b200e7d9a78a)
FetchContent_MakeAvailable(spirv-headers)
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "")
set(SPIRV_CHECK_CONTEXT OFF CACHE BOOL "")
set(SPIRV_SKIP_TESTS ON CACHE BOOL "")
set(SPIRV_WERROR OFF CACHE BOOL "")
FetchContent_Declare(spirv-tools
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
        GIT_TAG 212895d4c2cd9f4900adba4514aa5ea2c8256f9f)
FetchContent_MakeAvailable(spirv-tools)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "")
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "")
set(ENABLE_CTEST OFF CACHE BOOL "")
FetchContent_Declare(glslang
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang
        GIT_TAG 5878bcb17e43bf22cecaf6095900a33ff7f53445)
FetchContent_MakeAvailable(glslang)
set(SHADERC_ENABLE_WERROR_COMPILE OFF CACHE BOOL "")
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "")
set(SHADERC_SKIP_TESTS ON CACHE BOOL "")
FetchContent_Declare(shaderc
        GIT_REPOSITORY https://github.com/google/shaderc
        GIT_TAG 3fe94c6a76d2b617e7fb20fdf3cde98001a274e3)
FetchContent_MakeAvailable(shaderc)

FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb
        GIT_TAG c9064e317699d2e495f36ba4f9ac037e88ee371a)
FetchContent_MakeAvailable(stb)
target_include_directories(Minote PRIVATE ${stb_SOURCE_DIR})

# Shader compilation

set(SHADER_SOURCES
        src/glsl/swapchainBlit.vert src/glsl/swapchainBlit.frag
        src/glsl/object.vert src/glsl/object.frag
        src/glsl/cubemap.vert src/glsl/cubemap.frag
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
            COMMAND glslc_exe $<IF:$<CONFIG:Debug>,-O0,-O> -mfmt=num -o ${SHADER_OUTPUT} ${PROJECT_SOURCE_DIR}/${SHADER_PATH}
            DEPENDS ${SHADER_PATH}
            VERBATIM)
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach (SHADER_PATH)

add_custom_target(Compile_shaders DEPENDS ${SHADER_OUTPUTS})
add_dependencies(Compile_shaders shaderc)
target_include_directories(Minote PRIVATE ${PROJECT_BINARY_DIR}/generated)
add_dependencies(Minote Compile_shaders)
