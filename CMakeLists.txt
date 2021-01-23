# Project information
cmake_minimum_required(VERSION 3.17)
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
pkg_search_module(GLM REQUIRED glm)
pkg_search_module(FMT REQUIRED fmt)

add_compile_definitions(VK_NO_PROTOTYPES)
add_compile_definitions(GLFW_INCLUDE_NONE)
add_compile_definitions(GLFW_INCLUDE_VULKAN)
add_compile_definitions(GLM_FORCE_SIZE_T_LENGTH)
add_compile_definitions(GLM_FORCE_UNRESTRICTED_GENTYPE)
add_compile_definitions(GLM_FORCE_LEFT_HANDED)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Compile shaders
set(SHADER_SOURCES
        src/glsl/present.vert src/glsl/present.frag
        src/glsl/object.vert src/glsl/object.frag
        src/glsl/bloom.vert src/glsl/bloom.frag)

foreach (SHADER_PATH ${SHADER_SOURCES})
    get_filename_component(SHADER_FILENAME ${SHADER_PATH} NAME)
    set(SHADER_OUTPUT ${PROJECT_BINARY_DIR}/generated/spv/${SHADER_FILENAME}.spv)
    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/generated/spv/"
            COMMAND glslc -O -mfmt=num -o ${SHADER_OUTPUT} ${PROJECT_SOURCE_DIR}/${SHADER_PATH}
            DEPENDS ${SHADER_PATH}
            VERBATIM)
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach (SHADER_PATH)

add_custom_target(Compile_shaders DEPENDS ${SHADER_OUTPUTS})

# Build the game
set(INTERNALLIBS
        lib/VulkanMemoryAllocator/vma.h lib/VulkanMemoryAllocator/vk_mem_alloc.h lib/VulkanMemoryAllocator/vk_mem_alloc.cpp
        lib/robin-hood-hashing/robin_hood.h
        lib/static_vector/fixed_capacity_vector
        lib/scope_guard/scope_guard.hpp
        lib/xassert/xassert.h lib/xassert/xassert.c
        lib/volk/volk.h lib/volk/volk.c
        lib/stb/stb_image.h lib/stb/stb_image.c)

if(WIN32)
    add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
else()
    add_compile_definitions(VK_USE_PLATFORM_XCB_KHR)
endif()
add_compile_definitions(VMA_STATIC_VULKAN_FUNCTIONS=0)
add_compile_definitions(VMA_DYNAMIC_VULKAN_FUNCTIONS=0)
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(VMA_STATS_STRING_ENABLED=1)
endif()

set(SOURCES
        src/base/concepts.hpp
        src/base/zip_view.hpp
        src/base/hashmap.hpp
        src/base/svector.hpp
        src/base/version.hpp
        src/base/assert.hpp
        src/base/defer.hpp
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
        src/sys/vk/framebuffer.hpp src/sys/vk/framebuffer.cpp
        src/sys/vk/descriptor.hpp src/sys/vk/descriptor.cpp
        src/sys/vk/commands.hpp src/sys/vk/commands.cpp
        src/sys/vk/pipeline.hpp src/sys/vk/pipeline.cpp
        src/sys/vk/buffer.hpp src/sys/vk/buffer.tpp src/sys/vk/buffer.cpp
        src/sys/vk/shader.hpp src/sys/vk/shader.cpp
        src/sys/vk/debug.hpp src/sys/vk/debug.cpp
        src/sys/vk/image.hpp src/sys/vk/image.cpp
        src/sys/vk/base.hpp
        src/sys/keyboard.hpp src/sys/keyboard.cpp
        src/sys/window.hpp src/sys/window.tpp src/sys/window.cpp
        src/sys/glfw.hpp src/sys/glfw.cpp
        src/gfx/swapchain.hpp src/gfx/swapchain.cpp
        src/gfx/technique.hpp src/gfx/technique.cpp
        src/gfx/indirect.hpp src/gfx/indirect.cpp
        src/gfx/samplers.hpp src/gfx/samplers.cpp
        src/gfx/material.hpp
        src/gfx/context.hpp src/gfx/context.cpp
        src/gfx/targets.hpp src/gfx/targets.cpp
        src/gfx/present.hpp src/gfx/present.cpp
        src/gfx/engine.hpp src/gfx/engine.cpp
        src/gfx/bloom.hpp src/gfx/bloom.cpp
        src/gfx/world.hpp src/gfx/world.cpp
        src/gfx/mesh.hpp src/gfx/mesh.cpp
        src/gfx/base.hpp
        src/mesh/block.hpp
        src/mesh/scene.hpp
        src/engine/mapper.hpp src/engine/mapper.tpp src/engine/mapper.cpp
        src/playstate.hpp src/playstate.cpp
        src/game.hpp src/game.cpp
        src/main.hpp src/main.cpp
        src/mino.hpp src/mino.tpp)

add_executable(Minote ${SOURCES} ${INTERNALLIBS})
add_dependencies(Minote Compile_shaders)
target_compile_options(Minote PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Wno-missing-field-initializers -fno-rtti>)

target_include_directories(Minote PRIVATE src)
target_include_directories(Minote PRIVATE lib)
target_include_directories(Minote PRIVATE ${PROJECT_BINARY_DIR}/generated)
target_include_directories(Minote PRIVATE ${GLFW_INCLUDE_DIRS})
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
