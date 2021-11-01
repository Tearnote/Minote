# Project information

cmake_minimum_required(VERSION 3.20)
project(Minote VERSION 0.0
        DESCRIPTION "Modern action puzzle game"
        LANGUAGES C CXX)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0117 NEW)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build settings

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX2 /fp:fast /GR- /GF")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2 /fp:fast /GR- /GF")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Gw /Gy")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Gw /Gy")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
endif()

set(BUILD_SHARED_LIBS OFF)

# Main executable

add_executable(Minote
        src/base/containers/hashmap.hpp
        src/base/containers/vector.hpp
        src/base/containers/string.hpp
        src/base/containers/array.hpp
        src/base/concepts.hpp
        src/base/format.hpp
        src/base/error.hpp
        src/base/tween.hpp
        src/base/types.hpp
        src/base/math.hpp src/base/math.tpp
        src/base/util.hpp
        src/base/time.hpp
        src/base/ease.hpp
        src/base/log.hpp src/base/log.cpp
        src/base/rng.hpp
        src/base/id.hpp
        src/sys/window.hpp src/sys/window.cpp
        src/sys/vulkan.hpp src/sys/vulkan.cpp
        src/sys/system.hpp src/sys/system.cpp
        src/gfx/resources/texture2dms.hpp src/gfx/resources/texture2dms.cpp
        src/gfx/resources/texture3d.hpp src/gfx/resources/texture3d.cpp
        src/gfx/resources/texture2d.hpp src/gfx/resources/texture2d.cpp
        src/gfx/resources/cubemap.hpp src/gfx/resources/cubemap.cpp
        src/gfx/resources/buffer.hpp src/gfx/resources/buffer.tpp
        src/gfx/resources/pool.hpp
        src/gfx/effects/cubeFilterCoeffs.hpp
        src/gfx/effects/antialiasing.hpp src/gfx/effects/antialiasing.cpp
        src/gfx/effects/instanceList.hpp src/gfx/effects/instanceList.cpp
        src/gfx/effects/cubeFilter.hpp src/gfx/effects/cubeFilter.cpp
        src/gfx/effects/visibility.hpp src/gfx/effects/visibility.cpp
        src/gfx/effects/tonemap.hpp src/gfx/effects/tonemap.cpp
        src/gfx/effects/bloom.hpp src/gfx/effects/bloom.cpp
        src/gfx/effects/clear.hpp src/gfx/effects/clear.cpp
        src/gfx/effects/pbr.hpp src/gfx/effects/pbr.cpp
        src/gfx/effects/sky.hpp src/gfx/effects/sky.cpp
        src/gfx/materials.hpp src/gfx/materials.tpp src/gfx/materials.cpp
        src/gfx/samplers.hpp
        src/gfx/objects.hpp src/gfx/objects.cpp
        src/gfx/meshes.hpp src/gfx/meshes.cpp
        src/gfx/engine.hpp src/gfx/engine.cpp
        src/gfx/camera.hpp src/gfx/camera.cpp
        src/gfx/imgui.hpp src/gfx/imgui.cpp
        src/gfx/world.hpp src/gfx/world.cpp
        src/gfx/util.hpp
        #src/playstate.hpp src/playstate.cpp
        src/assets.hpp src/assets.tpp src/assets.cpp
        src/mapper.hpp src/mapper.cpp
        src/config.hpp
        src/game.hpp src/game.cpp
        src/main.hpp src/main.cpp
        src/mino.hpp src/mino.tpp)
target_precompile_headers(Minote PRIVATE
        src/base/containers/hashmap.hpp
        src/base/containers/vector.hpp
        src/base/containers/string.hpp
        src/base/containers/array.hpp
        src/base/concepts.hpp
        src/base/format.hpp
        src/base/error.hpp
        src/base/types.hpp
        src/base/math.hpp
        src/base/util.hpp
        src/base/log.hpp
        src/config.hpp)
target_include_directories(Minote PRIVATE src)
target_link_libraries(Minote PRIVATE winmm)

# Libraries

find_package(Vulkan REQUIRED)

include(FetchContent)

target_compile_definitions(Minote PRIVATE VK_NO_PROTOTYPES)
if(WIN32)
    target_compile_definitions(Minote PRIVATE VK_USE_PLATFORM_WIN32_KHR)
else()
    target_compile_definitions(Minote PRIVATE VK_USE_PLATFORM_XCB_KHR)
endif()

set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "")
FetchContent_Declare(volk
        GIT_REPOSITORY https://github.com/zeux/volk
        GIT_TAG 66169849935bd85f329c513dd01c28b0a02bfe33)
FetchContent_MakeAvailable(volk)
target_compile_definitions(volk PUBLIC VK_NO_PROTOTYPES)
target_include_directories(volk PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(Minote PRIVATE volk)

set(VUK_LINK_TO_LOADER OFF CACHE BOOL "")
set(VUK_USE_SHADERC OFF CACHE BOOL "")
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    set(VUK_USE_TRACY OFF CACHE BOOL "")
endif()
FetchContent_Declare(vuk
        GIT_REPOSITORY https://github.com/Tearnote/vuk
        GIT_TAG 168dbdf1bf019a693d10fc4da11ee07e5d93230d)
FetchContent_MakeAvailable(vuk)
target_compile_definitions(vuk PUBLIC VUK_CUSTOM_VULKAN_HEADER="volk.h")
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

set(OPT_DEF_LIBC ON CACHE BOOL "")
FetchContent_Declare(sdl
        GIT_REPOSITORY https://github.com/libsdl-org/SDL
        GIT_TAG release-2.0.14)
FetchContent_MakeAvailable(sdl)
target_link_libraries(Minote PRIVATE SDL2-static)

FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG 00d570e280533bca9cf6e5652d0f2eb131d032e7)
FetchContent_MakeAvailable(imgui)
add_library(imgui
        ${imgui_SOURCE_DIR}/imgui.h ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imconfig.h ${imgui_SOURCE_DIR}/imgui_internal.h
        ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl.h ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl.cpp)
target_compile_definitions(imgui PRIVATE IMGUI_DISABLE_WIN32_FUNCTIONS)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PRIVATE SDL2-static)
target_link_libraries(Minote PRIVATE imgui)

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
add_library(cgltf INTERFACE ${cgltf_SOURCE_DIR}/cgltf.h ${cgltf_SOURCE_DIR}/cgltf_write.h)
target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})
target_link_libraries(Minote PRIVATE cgltf)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    set(TRACY_ENABLE OFF CACHE BOOL "")
    set(TRACY_NO_FRAME_IMAGE ON CACHE BOOL "")
    set(TRACY_ONLY_LOCALHOST ON CACHE BOOL "")
    set(TRACY_NO_BROADCAST ON CACHE BOOL "")
else()
    set(TRACY_ENABLE OFF CACHE BOOL "")
endif()
FetchContent_Declare(tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy
        GIT_TAG d011d65b3c4cd80465030e3cf816d258d4a0eea1)
FetchContent_MakeAvailable(tracy)
target_link_libraries(TracyClient PRIVATE volk)
target_link_libraries(vuk PRIVATE TracyClient)
target_link_libraries(Minote PRIVATE TracyClient)

if(NOT TARGET robin_hood) # vuk already includes this
    FetchContent_Declare(robin_hood
            GIT_REPOSITORY https://github.com/martinus/robin-hood-hashing/
            GIT_TAG 3.11.3)
    FetchContent_MakeAvailable(robin_hood)
endif()
target_link_libraries(Minote PRIVATE robin_hood)

FetchContent_Declare(itlib
        GIT_REPOSITORY https://github.com/iboB/itlib
        GIT_TAG v1.3.0)
FetchContent_MakeAvailable(itlib)
target_compile_definitions(itlib INTERFACE ITLIB_STATIC_VECTOR_ERROR_HANDLING=ITLIB_STATIC_VECTOR_ERROR_HANDLING_THROW)
target_compile_definitions(itlib INTERFACE ITLIB_SMALL_VECTOR_ERROR_HANDLING=ITLIB_SMALL_VECTOR_ERROR_HANDLING_THROW)
target_link_libraries(Minote PRIVATE itlib)

FetchContent_Declare(quill
        GIT_REPOSITORY https://github.com/odygrd/quill
        GIT_TAG fa9b53f49732765ed242b7372ec4197e89f381ee)
FetchContent_MakeAvailable(quill)
target_link_libraries(Minote PRIVATE quill::quill)

FetchContent_Declare(gcem
        GIT_REPOSITORY https://github.com/kthohr/gcem
        GIT_TAG 8d0e62d9c9c47dfad2cb2d800281908cf7ff16c6)
FetchContent_MakeAvailable(gcem)
target_link_libraries(Minote PRIVATE gcem)

# Shader compilation

set(SHADER_SOURCES
        src/glsl/sky/skyGenAerialPerspective.comp
        src/glsl/sky/skyGenMultiScattering.comp
        src/glsl/sky/skyGenTransmittance.comp
        src/glsl/sky/skyGenSunLuminance.comp
        src/glsl/sky/skyDrawCubemap.comp
        src/glsl/sky/skyGenSkyView.comp
        src/glsl/sky/skyDrawQuad.comp
        src/glsl/sky/skyDraw.comp
        src/glsl/instanceSortCount.comp
        src/glsl/instanceSortWrite.comp
        src/glsl/instanceSortScan.comp
        src/glsl/bloomDownKaris.comp
        src/glsl/cubePostfilter.comp
        src/glsl/cubePrefilter.comp
        src/glsl/transformConv.comp
        src/glsl/quadAssign.comp
        src/glsl/quadResolve.comp
        src/glsl/frustumCull.comp
        src/glsl/visibility.vert src/glsl/visibility.frag
        src/glsl/worklistMS.comp
        src/glsl/bloomDown.comp
        src/glsl/worklist.comp
        src/glsl/tonemap.comp
        src/glsl/bloomUp.comp
        src/glsl/imgui.vert src/glsl/imgui.frag
        src/glsl/pbrQuad.comp
        src/glsl/pbr.comp)

foreach(SHADER_PATH ${SHADER_SOURCES})
    get_filename_component(SHADER_FILENAME ${SHADER_PATH} NAME)
    set(SHADER_OUTPUT ${PROJECT_BINARY_DIR}/generated/spv/${SHADER_FILENAME}.spv)
    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/generated/spv/"
            COMMAND $ENV{VULKAN_SDK}/Bin/glslc.exe --target-env=vulkan1.2 -mfmt=num $<$<NOT:$<CONFIG:Debug>>:-O> $<$<NOT:$<CONFIG:Release>>:-g> -MD -MF ${SHADER_OUTPUT}.d -o ${SHADER_OUTPUT} ${PROJECT_SOURCE_DIR}/${SHADER_PATH}
            DEPENDS ${SHADER_PATH}
            DEPFILE ${SHADER_OUTPUT}.d
            VERBATIM COMMAND_EXPAND_LISTS)
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(Compile_shaders DEPENDS ${SHADER_OUTPUTS})
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
        models/sphere.glb
        models/testscene.glb)
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
