cmake_minimum_required(VERSION 3.20)
project(Minote
	VERSION 0.0
	DESCRIPTION "A personal realtime renderer project"
	LANGUAGES C CXX)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0117 NEW)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Global build settings
add_compile_options("-march=x86-64;-mmmx;-msse;-msse2;-msse3;-mssse3;-msse4.1;-msse4.2;-mavx;-ffast-math")
add_compile_options("$<$<CONFIG:Debug,RelWithDebInfo>:-g>")
add_compile_options("$<$<CONFIG:RelWithDebInfo,Release>:-O3;-ffunction-sections;-Wl,--gc-sections>")
add_compile_options("$<$<CONFIG:Release>:-s>")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION "$<CONFIG:Release>")
if(WIN32)
	add_compile_options("-mwindows")
endif()
# Dynamic+lld for dev builds, static+ld for release
# https://github.com/msys2/MINGW-packages/issues/12050
set(BUILD_SHARED_LIBS OFF)
add_link_options("-static-libgcc")
add_link_options("$<$<CONFIG:Debug,RelWithDebInfo>:-fuse-ld=lld>")
add_link_options("$<$<CONFIG:Release>:-static;-static-libstdc++>")
# Communicate build type to the project
add_compile_definitions("BUILD_TYPE=$<$<CONFIG:Debug>:BUILD_DEBUG>$<$<CONFIG:RelWithDebInfo>:BUILD_RELDEB>$<$<CONFIG:Release>:BUILD_RELEASE>")

# Main executable
add_executable(Minote
	src/util/concepts.hpp
	src/util/optional.hpp
	src/util/service.hpp
	src/util/hashmap.hpp
	src/util/verify.hpp
	src/util/vector.hpp
	src/util/string.hpp
	src/util/format.hpp
	src/util/array.hpp
	src/util/error.hpp
	src/util/tween.hpp
	src/util/types.hpp
	src/util/math.hpp src/util/math.tpp
	src/util/span.hpp
	src/util/util.hpp
	src/util/time.hpp
	src/util/ease.hpp
	src/util/log.hpp src/util/log.cpp
	src/util/rng.hpp
	src/util/id.hpp
	src/tools/modelSchema.hpp
	src/sys/vulkan.hpp src/sys/vulkan.cpp
	src/sys/system.hpp src/sys/system.cpp
	src/gfx/effects/instanceList.hpp src/gfx/effects/instanceList.cpp
	src/gfx/effects/visibility.hpp src/gfx/effects/visibility.cpp
	src/gfx/effects/tonemap.hpp src/gfx/effects/tonemap.cpp
	src/gfx/effects/bloom.hpp src/gfx/effects/bloom.cpp
	src/gfx/effects/shade.hpp src/gfx/effects/shade.cpp
	src/gfx/effects/sky.hpp src/gfx/effects/sky.cpp
	src/gfx/effects/spd.hpp src/gfx/effects/spd.cpp
	src/gfx/effects/hiz.hpp src/gfx/effects/hiz.cpp
	src/gfx/samplers.hpp
	src/gfx/renderer.hpp src/gfx/renderer.cpp
	src/gfx/resource.hpp
	src/gfx/objects.hpp src/gfx/objects.cpp
	src/gfx/models.hpp src/gfx/models.cpp
	src/gfx/camera.hpp src/gfx/camera.cpp
	src/gfx/shader.hpp
	src/gfx/imgui.hpp src/gfx/imgui.cpp
	src/gfx/util.hpp
	src/freecam.hpp src/freecam.cpp
	src/assets.hpp src/assets.tpp src/assets.cpp
	src/mapper.hpp src/mapper.cpp
	src/scenes.hpp src/scenes.cpp
	src/config.hpp
	src/game.hpp src/game.cpp
	src/main.hpp src/main.cpp)
target_precompile_headers(Minote PRIVATE
	src/util/concepts.hpp
	src/util/optional.hpp
	src/util/service.hpp
	src/util/hashmap.hpp
	src/util/verify.hpp
	src/util/vector.hpp
	src/util/string.hpp
	src/util/format.hpp
	src/util/array.hpp
	src/util/error.hpp
	src/util/types.hpp
	src/util/span.hpp
	src/util/math.hpp
	src/util/util.hpp
	src/util/time.hpp
	src/util/log.hpp
	src/util/id.hpp
	src/config.hpp)
target_include_directories(Minote PRIVATE src)

# Libraries
include(cmake/Dependencies.cmake)

target_link_libraries(Minote PRIVATE vk-bootstrap)
target_link_libraries(Minote PRIVATE quill::quill)
target_link_libraries(Minote PRIVATE SDL2-static)
target_link_libraries(Minote PRIVATE robin_hood)
target_link_libraries(Minote PRIVATE fmt::fmt)
target_link_libraries(Minote PRIVATE incbin)
target_link_libraries(Minote PRIVATE assert)
target_link_libraries(Minote PRIVATE sqlite)
target_link_libraries(Minote PRIVATE imgui)
target_link_libraries(Minote PRIVATE itlib)
target_link_libraries(Minote PRIVATE mpack)
target_link_libraries(Minote PRIVATE volk)
target_link_libraries(Minote PRIVATE gcem)
target_link_libraries(Minote PRIVATE spd)
target_link_libraries(Minote PRIVATE pcg)
target_link_libraries(Minote PRIVATE vuk)
if(WIN32)
	target_link_libraries(Minote PRIVATE winmm)
endif()

# Packaged content
include(cmake/Shaders.cmake)
target_link_libraries(Minote PRIVATE MinoteShaders)
include(cmake/Assets.cmake)
add_dependencies(Minote MinoteAssets)
