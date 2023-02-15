cmake_minimum_required(VERSION 3.23)
project(Minote
	VERSION 0.0
	DESCRIPTION "A personal realtime renderer project"
	LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Global build settings
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug,RelWithDebInfo>:Debug>")
string(REPLACE "/Zi" "/ZI" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
string(REPLACE "/Zi" "/ZI" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
add_compile_options("/arch:AVX2;/fp:fast;/EHsc;/wd5105")
add_compile_options("$<$<CONFIG:Debug>:/RTC1>")
add_compile_options("$<$<CONFIG:Debug,RelWithDebInfo>:/JMC>")
add_compile_options("$<$<CONFIG:RelWithDebInfo>:/Zi>")
add_compile_options("$<$<CONFIG:RelWithDebInfo,Release>:/O2>")
add_compile_options("$<$<CONFIG:Release>:/Gw>")
#TODO LTO commented out, pending VMA fix
#add_compile_options("$<$<CONFIG:Release>:/GL>")
#add_link_options("$<$<CONFIG:Release>:/LTCG>")
# Communicate build type to the project
add_compile_definitions("BUILD_TYPE=$<$<CONFIG:Debug>:BUILD_DEBUG>$<$<CONFIG:RelWithDebInfo>:BUILD_RELDEB>$<$<CONFIG:Release>:BUILD_RELEASE>")

# Main executable
add_executable(Minote
	src/stx/concepts.hpp
	src/stx/hashmap.hpp
	src/stx/except.hpp
	src/stx/vector.hpp
	src/stx/time.hpp
	src/util/service.hpp
	src/util/verify.hpp
	src/util/tween.hpp
	src/util/util.hpp
	src/util/ease.hpp
	src/util/rng.hpp
	src/util/id.hpp
	src/tools/modelSchema.hpp
	src/sys/vulkan.hpp src/sys/vulkan.cpp
	src/sys/system.hpp src/sys/system.cpp
	src/gfx/effects/tonemap.hpp src/gfx/effects/tonemap.cpp
	src/gfx/effects/bloom.hpp src/gfx/effects/bloom.cpp
	src/gfx/effects/sky.hpp src/gfx/effects/sky.cpp
	src/gfx/effects/spd.hpp src/gfx/effects/spd.cpp
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
	src/types.hpp
	src/math.hpp src/math.tpp
	src/game.hpp src/game.cpp
	src/main.hpp src/main.cpp
	src/log.hpp src/log.cpp)
target_include_directories(Minote PRIVATE src)

# Libraries
include(cmake/Dependencies.cmake)

target_link_options(Minote PRIVATE "$<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>")
target_link_libraries(Minote PRIVATE SDL2::SDL2-static)
target_link_libraries(Minote PRIVATE SDL2::SDL2main)
target_link_libraries(Minote PRIVATE vk-bootstrap)
target_link_libraries(Minote PRIVATE robin_hood)
target_link_libraries(Minote PRIVATE incbin)
target_link_libraries(Minote PRIVATE assert)
target_link_libraries(Minote PRIVATE sqlite)
target_link_libraries(Minote PRIVATE fmtlog)
target_link_libraries(Minote PRIVATE imgui)
target_link_libraries(Minote PRIVATE itlib)
target_link_libraries(Minote PRIVATE mpack)
target_link_libraries(Minote PRIVATE volk)
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
