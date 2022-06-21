# Project information

cmake_minimum_required(VERSION 3.20)
project(Minote
	VERSION 0.0
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

add_compile_options("-march=x86-64;-mmmx;-msse;-msse2;-msse3;-mssse3;-msse4.1;-msse4.2;-mavx;-ffast-math")

add_compile_options("$<$<CONFIG:Debug,RelWithDebInfo>:-g>")

add_compile_options("$<$<CONFIG:RelWithDebInfo,Release>:-O3;-ffunction-sections;-Wl,--gc-sections>")
add_compile_options("$<$<CONFIG:Release>:-s>")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION "$<CONFIG:Release>")

if(WIN32)
	add_compile_options("-mwindows")
endif()

set(BUILD_SHARED_LIBS OFF)
add_link_options("-static;-static-libgcc;-static-libstdc++")
add_link_options("$<$<CONFIG:Debug,RelWithDebInfo>:-fuse-ld=lld>")

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
	src/tools/modelSchema.hpp
	src/sys/window.hpp src/sys/window.cpp
	src/sys/vulkan.hpp src/sys/vulkan.cpp
	src/sys/system.hpp src/sys/system.cpp
	# src/gfx/effects/cubeFilterCoeffs.hpp
	# src/gfx/effects/instanceList.hpp src/gfx/effects/instanceList.cpp
	# src/gfx/effects/quadBuffer.hpp src/gfx/effects/quadBuffer.cpp
	# src/gfx/effects/cubeFilter.hpp src/gfx/effects/cubeFilter.cpp
	# src/gfx/effects/visibility.hpp src/gfx/effects/visibility.cpp
	# src/gfx/effects/tonemap.hpp src/gfx/effects/tonemap.cpp
	# src/gfx/effects/bloom.hpp src/gfx/effects/bloom.cpp
	# src/gfx/effects/clear.hpp src/gfx/effects/clear.cpp
	# src/gfx/effects/bvh.hpp src/gfx/effects/bvh.cpp
	# src/gfx/effects/pbr.hpp src/gfx/effects/pbr.cpp
	# src/gfx/effects/sky.hpp src/gfx/effects/sky.cpp
	# src/gfx/effects/hiz.hpp src/gfx/effects/hiz.cpp
	# src/gfx/samplers.hpp
	src/gfx/objects.hpp src/gfx/objects.cpp
	src/gfx/models.hpp src/gfx/models.cpp
	src/gfx/engine.hpp src/gfx/engine.cpp
	src/gfx/camera.hpp src/gfx/camera.cpp
	src/gfx/imgui.hpp src/gfx/imgui.cpp
	src/gfx/world.hpp src/gfx/world.cpp
	# src/gfx/frame.hpp src/gfx/frame.cpp
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
	GIT_TAG 5d7c78443c0b435556b16f63ebac8f25b5f5ba6e)
FetchContent_MakeAvailable(volk)
target_compile_definitions(volk PUBLIC VK_NO_PROTOTYPES)
target_include_directories(volk PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(Minote PRIVATE volk)

set(VUK_LINK_TO_LOADER OFF CACHE BOOL "")
set(VUK_USE_SHADERC OFF CACHE BOOL "")
set(VUK_USE_DXC OFF CACHE BOOL "")
FetchContent_Declare(vuk
	GIT_REPOSITORY https://github.com/martty/vuk
	GIT_TAG 3ed377a5a02399fd44fa28ca4e2b3caeee8305de)
FetchContent_MakeAvailable(vuk)
target_compile_definitions(vuk PUBLIC VUK_CUSTOM_VULKAN_HEADER="volk.h")
target_link_libraries(vuk PRIVATE volk)
target_link_libraries(Minote PRIVATE vuk)

FetchContent_Declare(vk-bootstrap
	GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
	GIT_TAG f847ab8566a81da84aa38afe7c19f3abf6210b98)
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
set(VIDEO_OPENGLES OFF CACHE BOOL "")
set(VIDEO_OPENGL OFF CACHE BOOL "")
set(SDL_DLOPEN OFF CACHE BOOL "")
set(RENDER_D3D OFF CACHE BOOL "")
set(DUMMYAUDIO OFF CACHE BOOL "")
set(DUMMYAUDIO OFF CACHE BOOL "")
set(DISKAUDIO OFF CACHE BOOL "")
set(DIRECTX OFF CACHE BOOL "")
set(WASAPI OFF CACHE BOOL "")
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

if(NOT TARGET robin_hood) # vuk already includes this
	FetchContent_Declare(robin_hood
		GIT_REPOSITORY https://github.com/martinus/robin-hood-hashing
		GIT_TAG 3.11.3)
	FetchContent_MakeAvailable(robin_hood)
endif()
target_link_libraries(Minote PRIVATE robin_hood)

FetchContent_Declare(itlib
	GIT_REPOSITORY https://github.com/iboB/itlib
	GIT_TAG v1.4.5)
FetchContent_MakeAvailable(itlib)
target_compile_definitions(itlib INTERFACE ITLIB_STATIC_VECTOR_ERROR_HANDLING=ITLIB_STATIC_VECTOR_ERROR_HANDLING_THROW)
target_compile_definitions(itlib INTERFACE ITLIB_SMALL_VECTOR_ERROR_HANDLING=ITLIB_SMALL_VECTOR_ERROR_HANDLING_THROW)
target_link_libraries(Minote PRIVATE itlib)

FetchContent_Declare(fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt
	GIT_TAG 8.1.1)
FetchContent_MakeAvailable(fmt)
target_link_libraries(Minote PRIVATE fmt::fmt)

set(QUILL_FMT_EXTERNAL ON CACHE BOOL "")
FetchContent_Declare(quill
	GIT_REPOSITORY https://github.com/Tearnote/quill
	GIT_TAG 98b09cf8b824ea3f6c8d7ea93e9c37bb94709f9c)
FetchContent_MakeAvailable(quill)
target_link_libraries(quill PRIVATE fmt::fmt)
target_link_libraries(Minote PRIVATE quill::quill)

FetchContent_Declare(gcem
	GIT_REPOSITORY https://github.com/kthohr/gcem
	GIT_TAG 8d0e62d9c9c47dfad2cb2d800281908cf7ff16c6)
FetchContent_MakeAvailable(gcem)
target_link_libraries(Minote PRIVATE gcem)

FetchContent_Declare(mpack
	URL https://github.com/ludocode/mpack/releases/download/v1.1/mpack-amalgamation-1.1.tar.gz)
FetchContent_MakeAvailable(mpack)
add_library(mpack
	${mpack_SOURCE_DIR}/src/mpack/mpack.h ${mpack_SOURCE_DIR}/src/mpack/mpack.c)
# Force C++-style handling of inline functions. Fixes link-time error with GCC
set_property(SOURCE ${mpack_SOURCE_DIR}/src/mpack/mpack.c PROPERTY LANGUAGE CXX)
target_include_directories(mpack PUBLIC ${mpack_SOURCE_DIR}/src)
target_link_libraries(Minote PRIVATE mpack)

FetchContent_Declare(meshoptimizer
	GIT_REPOSITORY https://github.com/zeux/meshoptimizer
	GIT_TAG 70b6cc38a64eeb549567599418bc4ed3ebd607da)
FetchContent_MakeAvailable(meshoptimizer)

FetchContent_Declare(bvh
	GIT_REPOSITORY https://github.com/madmann91/bvh
	GIT_TAG 02749bca616260d2df1a8de69e76075b2c8a526b)
FetchContent_MakeAvailable(bvh)
target_link_libraries(Minote PRIVATE bvh)

# Shader compilation

set(SHADER_DIR_PREFIX src/glsl)
set(SHADER_SOURCES
	instanceList/cullMeshlets.comp
	instanceList/genIndices.comp
	cubeFilter/post.comp
	cubeFilter/pre.comp
	visibility/worklist.comp
	visibility/visbuf.vert visibility/visbuf.frag
	bloom/downKaris.comp
	bloom/down.comp
	bloom/up.comp
	quad/genBuffers.comp
	quad/clusterize.comp
	quad/resolve.comp
	bvh/debugAABB.vert
	bvh/debugAABB.frag
	sky/genAerialPerspective.comp
	sky/genMultiScattering.comp
	sky/genTransmittance.comp
	sky/genSunLuminance.comp
	sky/drawCubemap.comp
	sky/genSkyView.comp
	sky/draw.comp
	hiz/first.comp
	hiz/mip.comp
	tonemap.comp
	imgui.vert imgui.frag
	pbr.comp)

foreach(SHADER_PATH ${SHADER_SOURCES})
	get_filename_component(SHADER_DIR ${SHADER_PATH} DIRECTORY)
	set(SHADER_OUTPUT ${PROJECT_BINARY_DIR}/$<CONFIG>/generated/spv/${SHADER_PATH}.spv)
	add_custom_command(
		OUTPUT ${SHADER_OUTPUT}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/$<CONFIG>/generated/spv/${SHADER_DIR}
		COMMAND $ENV{VULKAN_SDK}/Bin/glslc.exe --target-env=vulkan1.2 -mfmt=num $<$<CONFIG:Debug,RelWithDebInfo>:-g> -MD -MF ${SHADER_OUTPUT}.d -o ${SHADER_OUTPUT} ${PROJECT_SOURCE_DIR}/${SHADER_DIR_PREFIX}/${SHADER_PATH}
		DEPENDS ${SHADER_DIR_PREFIX}/${SHADER_PATH}
		DEPFILE ${SHADER_OUTPUT}.d
		VERBATIM COMMAND_EXPAND_LISTS)
	list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(Compile_shaders DEPENDS ${SHADER_OUTPUTS})
target_include_directories(Minote PRIVATE ${PROJECT_BINARY_DIR}/$<CONFIG>/generated)
add_dependencies(Minote Compile_shaders)

# Asset pipeline

add_executable(Model_conv
	src/tools/modelSchema.hpp
	src/tools/modelConv.cpp)
target_include_directories(Model_conv PRIVATE src)
target_link_libraries(Model_conv PRIVATE meshoptimizer)
target_link_libraries(Model_conv PRIVATE quill::quill)
target_link_libraries(Model_conv PRIVATE itlib)
target_link_libraries(Model_conv PRIVATE cgltf)
target_link_libraries(Model_conv PRIVATE mpack)
target_link_libraries(Model_conv PRIVATE gcem)

set(ASSET_OUTPUT ${PROJECT_BINARY_DIR}/$<CONFIG>/assets.db)
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
		OUTPUT ${PROJECT_BINARY_DIR}/$<CONFIG>/models/${MODEL_NAME}.model
		COMMAND Model_conv ${PROJECT_SOURCE_DIR}/${MODEL_PATH} ${PROJECT_BINARY_DIR}/$<CONFIG>/models/${MODEL_NAME}.model
		DEPENDS ${PROJECT_SOURCE_DIR}/${MODEL_PATH}
		DEPENDS Model_conv
		VERBATIM COMMAND_EXPAND_LISTS)
	add_custom_command(
		OUTPUT ${ASSET_OUTPUT} APPEND
		COMMAND sqlite-shell ${ASSET_OUTPUT} "INSERT OR REPLACE INTO models(name, data) VALUES('${MODEL_NAME}', readfile('${PROJECT_BINARY_DIR}/$<CONFIG>/models/${MODEL_NAME}.model'))"
		DEPENDS ${PROJECT_BINARY_DIR}/$<CONFIG>/models/${MODEL_NAME}.model
		DEPENDS sqlite-shell
		VERBATIM COMMAND_EXPAND_LISTS)
endforeach()

add_custom_target(Package_assets DEPENDS ${ASSET_OUTPUT})
add_dependencies(Minote Package_assets)
