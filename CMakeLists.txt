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

option(MINOTE_USE_VULKAN_SDK "Use the Vulkan SDK for libraries instead of manual fetching" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -Wl,--gc-sections -mfpmath=sse -mmmx -msse -msse2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -Wl,--gc-sections -mfpmath=sse -mmmx -msse -msse2")
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
    endif()
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
        src/gfx/instances.hpp src/gfx/instances.cpp
        src/gfx/pipelines.hpp src/gfx/pipelines.cpp
        src/gfx/iblCoeffs.hpp
        src/gfx/indirect.hpp src/gfx/indirect.cpp
        src/gfx/meshes.hpp src/gfx/meshes.cpp
        src/gfx/engine.hpp src/gfx/engine.cpp
        src/gfx/imgui.hpp src/gfx/imgui.cpp
        src/gfx/world.hpp
        src/gfx/base.hpp
        src/gfx/ibl.hpp src/gfx/ibl.cpp
        src/gfx/sky.hpp src/gfx/sky.cpp
        #src/playstate.hpp src/playstate.cpp
        src/mapper.hpp src/mapper.tpp src/mapper.cpp
        src/assets.hpp src/assets.tpp src/assets.cpp
        src/config.hpp
        src/game.hpp src/game.cpp
        src/main.hpp src/main.cpp
        src/mino.hpp src/mino.tpp)
target_compile_options(Minote PRIVATE -Wall -Wextra -Wno-missing-field-initializers -fno-rtti)
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
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
        GIT_REPOSITORY https://github.com/Tearnote/vuk
        GIT_TAG 39d8a4da4fce0db0c671fcc6158c6070b31998b5)
FetchContent_MakeAvailable(vuk)
target_compile_definitions(vuk PRIVATE VK_NO_PROTOTYPES)
target_compile_definitions(vuk PRIVATE VUK_CUSTOM_VULKAN_HEADER="volk.h")
target_link_libraries(vuk PRIVATE volk)
target_link_libraries(Minote PRIVATE vuk)

FetchContent_Declare(vk-bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
        GIT_TAG 36eff8fa4222b2225e349a44dde0402f0f4a277b)
FetchContent_MakeAvailable(vk-bootstrap)
target_link_libraries(Minote PRIVATE vk-bootstrap)

FetchContent_Declare(pcg
        GIT_REPOSITORY https://github.com/imneme/pcg-c-basic
        GIT_TAG bc39cd76ac3d541e618606bcc6e1e5ba5e5e6aa3)
FetchContent_MakeAvailable(pcg)
add_library(pcg ${pcg_SOURCE_DIR}/pcg_basic.h ${pcg_SOURCE_DIR}/pcg_basic.c)
target_include_directories(pcg PUBLIC ${pcg_SOURCE_DIR})
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
target_compile_definitions(Minote PRIVATE GLM_FORCE_XYZW_ONLY)
FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 4abd46483ab3cadda5cdb40dcd28e6f3b789d621)
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
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PRIVATE glfw)
target_link_libraries(Minote PRIVATE imgui)

if(NOT MINOTE_USE_VULKAN_SDK)
    set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "")
    set(SPIRV_HEADERS_SKIP_INSTALL ON CACHE BOOL "")
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
    set(SHADERC_SKIP_INSTALL ON CACHE BOOL "")
    set(SHADERC_SKIP_TESTS ON CACHE BOOL "")
    FetchContent_Declare(shaderc
        GIT_REPOSITORY https://github.com/google/shaderc
        GIT_TAG 3fe94c6a76d2b617e7fb20fdf3cde98001a274e3)
    FetchContent_MakeAvailable(shaderc)
endif()

FetchContent_Declare(sqlite
        URL https://www.sqlite.org/2021/sqlite-amalgamation-3350400.zip)
FetchContent_MakeAvailable(sqlite)
add_library(sqlite
        ${sqlite_SOURCE_DIR}/sqlite3.h ${sqlite_SOURCE_DIR}/sqlite3.c
        ${sqlite_SOURCE_DIR}/sqlite3ext.h)
target_include_directories(sqlite PUBLIC ${sqlite_SOURCE_DIR})
target_link_libraries(Minote PRIVATE sqlite)
add_executable(sqlite-shell
        ${sqlite_SOURCE_DIR}/shell.c)
target_link_libraries(sqlite-shell PRIVATE sqlite)

FetchContent_Declare(cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf
        GIT_TAG 1bdc84d3bb81fa69bcf71ed5cafe63e58df1448a)
FetchContent_MakeAvailable(cgltf)
add_library(cgltf ${cgltf_SOURCE_DIR}/cgltf.h ${cgltf_SOURCE_DIR}/cgltf_write.h)
set_target_properties(cgltf PROPERTIES LINKER_LANGUAGE C)
target_include_directories(cgltf PUBLIC ${cgltf_SOURCE_DIR})
target_link_libraries(Minote PRIVATE cgltf)

# Shader compilation

set(SHADER_SOURCES
        src/glsl/sky/skyGenAerialPerspective.comp
        src/glsl/sky/skyGenMultiScattering.comp
        src/glsl/sky/skyGenTransmittance.comp
        src/glsl/sky/skyDrawCubemap.comp
        src/glsl/sky/skyGenSkyView.comp
        src/glsl/sky/skyDraw.vert src/glsl/sky/skyDraw.frag
        src/glsl/iblPostfilter.comp
        src/glsl/iblPrefilter.comp
        src/glsl/zprepass.vert src/glsl/zprepass.frag
        src/glsl/tonemap.vert src/glsl/tonemap.frag
        src/glsl/object.vert src/glsl/object.frag
        src/glsl/imgui.vert src/glsl/imgui.frag
        src/glsl/cull.comp)

foreach(SHADER_PATH ${SHADER_SOURCES})
    get_filename_component(SHADER_FILENAME ${SHADER_PATH} NAME)
    set(SHADER_OUTPUT ${PROJECT_BINARY_DIR}/generated/spv/${SHADER_FILENAME}.spv)
    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/generated/spv/"
            COMMAND $<IF:$<BOOL:${MINOTE_USE_VULKAN_SDK}>,glslc,glslc_exe> --target-env=vulkan1.2 -mfmt=num $<$<NOT:$<CONFIG:Release>>:-g> -o ${SHADER_OUTPUT} ${PROJECT_SOURCE_DIR}/${SHADER_PATH}
            DEPENDS ${SHADER_PATH}
            VERBATIM COMMAND_EXPAND_LISTS)
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(Compile_shaders DEPENDS ${SHADER_OUTPUTS})
if(NOT MINOTE_USE_VULKAN_SDK)
add_dependencies(Compile_shaders shaderc)
endif()
target_include_directories(Minote PRIVATE ${PROJECT_BINARY_DIR}/generated)
add_dependencies(Minote Compile_shaders)

# Asset packaging

set(ASSET_OUTPUT ${PROJECT_BINARY_DIR}/assets.db)
add_custom_command(
        OUTPUT ${ASSET_OUTPUT}
        COMMAND sqlite-shell ${ASSET_OUTPUT} "VACUUM"
        VERBATIM)

set(MODELS
        models/block.glb
        models/sphere.glb)
add_custom_command(
        OUTPUT ${ASSET_OUTPUT} APPEND
        COMMAND sqlite-shell ${ASSET_OUTPUT} "CREATE TABLE IF NOT EXISTS models(name TEXT UNIQUE, data BLOB)"
        VERBATIM)
foreach(MODEL_PATH ${MODELS})
    get_filename_component(MODEL_NAME ${MODEL_PATH} NAME_WLE)
    add_custom_command(
            OUTPUT ${ASSET_OUTPUT} APPEND
            COMMAND sqlite-shell ${ASSET_OUTPUT} "INSERT OR REPLACE INTO models(name, data) VALUES('${MODEL_NAME}', readfile('${PROJECT_SOURCE_DIR}/${MODEL_PATH}'))"
            DEPENDS ${MODEL_PATH}
            VERBATIM COMMAND_EXPAND_LISTS)
endforeach()

add_custom_target(Package_assets DEPENDS ${ASSET_OUTPUT})
add_dependencies(Package_assets sqlite-shell)

add_dependencies(Minote Package_assets)
