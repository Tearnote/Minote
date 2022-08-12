include_guard()

include(FetchContent)
find_package(Vulkan REQUIRED)

set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "")
FetchContent_Declare(volk
	GIT_REPOSITORY https://github.com/zeux/volk
	GIT_TAG 315929ebf428a0b924c1cff5803b69891f71bf5b)
FetchContent_MakeAvailable(volk)
target_compile_definitions(volk PUBLIC VK_NO_PROTOTYPES)
target_include_directories(volk PUBLIC ${Vulkan_INCLUDE_DIRS})
if(WIN32)
	target_compile_definitions(volk PUBLIC VK_USE_PLATFORM_WIN32_KHR)
else()
	target_compile_definitions(volk PUBLIC VK_USE_PLATFORM_XCB_KHR)
endif()

set(VUK_LINK_TO_LOADER OFF CACHE BOOL "")
set(VUK_USE_SHADERC OFF CACHE BOOL "")
set(VUK_USE_DXC OFF CACHE BOOL "")
FetchContent_Declare(vuk
	GIT_REPOSITORY https://github.com/martty/vuk
	GIT_TAG 46b5ea6597015b360bffff57ffa6c9bcccf2f5a6)
FetchContent_MakeAvailable(vuk)
target_compile_definitions(vuk PUBLIC VUK_CUSTOM_VULKAN_HEADER="volk.h")
target_link_libraries(vuk PRIVATE volk)

FetchContent_Declare(vk-bootstrap
	GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
	GIT_TAG 00cf404e7b37fa5df41c1d7c369f1fa33efbe4d2)
FetchContent_MakeAvailable(vk-bootstrap)

FetchContent_Declare(pcg
	GIT_REPOSITORY https://github.com/imneme/pcg-c-basic
	GIT_TAG bc39cd76ac3d541e618606bcc6e1e5ba5e5e6aa3)
FetchContent_MakeAvailable(pcg)
add_library(pcg ${pcg_SOURCE_DIR}/pcg_basic.h ${pcg_SOURCE_DIR}/pcg_basic.c)
target_include_directories(pcg PUBLIC ${pcg_SOURCE_DIR})

FetchContent_Declare(assert
	GIT_REPOSITORY https://github.com/jeremy-rifkin/libassert
	GIT_TAG 599a885a57050cd100ed771a189649ad7a603d4f)
FetchContent_MakeAvailable(assert)
target_link_libraries(assert PRIVATE dbghelp) # library incorrectly assumes only MSVC needs it

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
	GIT_TAG release-2.0.16)
FetchContent_MakeAvailable(sdl)

FetchContent_Declare(imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui
	GIT_TAG v1.88)
FetchContent_MakeAvailable(imgui)
add_library(imgui
	${imgui_SOURCE_DIR}/imgui.h ${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imconfig.h ${imgui_SOURCE_DIR}/imgui_internal.h
	${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_sdl.h ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl.cpp)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_KEYIO)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_WIN32_FUNCTIONS)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PRIVATE SDL2-static)

FetchContent_Declare(sqlite
	URL https://www.sqlite.org/2022/sqlite-amalgamation-3390200.zip)
FetchContent_MakeAvailable(sqlite)
add_library(sqlite
	${sqlite_SOURCE_DIR}/sqlite3.h ${sqlite_SOURCE_DIR}/sqlite3.c
	${sqlite_SOURCE_DIR}/sqlite3ext.h)
target_include_directories(sqlite PUBLIC ${sqlite_SOURCE_DIR})
add_executable(sqlite-shell
${sqlite_SOURCE_DIR}/shell.c)
target_link_libraries(sqlite-shell PRIVATE sqlite)

FetchContent_Declare(cgltf
	GIT_REPOSITORY https://github.com/jkuhlmann/cgltf
	GIT_TAG cd37936659b448554f612a145d05e76ff27832a6)
FetchContent_MakeAvailable(cgltf)
add_library(cgltf INTERFACE ${cgltf_SOURCE_DIR}/cgltf.h ${cgltf_SOURCE_DIR}/cgltf_write.h)
target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})

if(NOT TARGET robin_hood) # vuk already includes this
	FetchContent_Declare(robin_hood
		GIT_REPOSITORY https://github.com/martinus/robin-hood-hashing
		GIT_TAG 3.11.5)
	FetchContent_MakeAvailable(robin_hood)
endif()

FetchContent_Declare(itlib
	GIT_REPOSITORY https://github.com/iboB/itlib
	GIT_TAG v1.5.2)
FetchContent_MakeAvailable(itlib)
target_compile_definitions(itlib INTERFACE ITLIB_STATIC_VECTOR_ERROR_HANDLING=ITLIB_STATIC_VECTOR_ERROR_HANDLING_THROW)
target_compile_definitions(itlib INTERFACE ITLIB_SMALL_VECTOR_ERROR_HANDLING=ITLIB_SMALL_VECTOR_ERROR_HANDLING_THROW)

FetchContent_Declare(fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt
	GIT_TAG 9.0.0)
FetchContent_MakeAvailable(fmt)

set(QUILL_FMT_EXTERNAL ON CACHE BOOL "")
FetchContent_Declare(quill
	GIT_REPOSITORY https://github.com/odygrd/quill
	GIT_TAG v2.1.0)
FetchContent_MakeAvailable(quill)
target_link_libraries(quill PRIVATE fmt::fmt)

FetchContent_Declare(gcem
	GIT_REPOSITORY https://github.com/kthohr/gcem
	GIT_TAG v1.15.0)
FetchContent_MakeAvailable(gcem)

FetchContent_Declare(mpack
	URL https://github.com/ludocode/mpack/releases/download/v1.1/mpack-amalgamation-1.1.tar.gz)
FetchContent_MakeAvailable(mpack)
add_library(mpack
	${mpack_SOURCE_DIR}/src/mpack/mpack.h ${mpack_SOURCE_DIR}/src/mpack/mpack.c)
# Force C++-style handling of inline functions. Fixes link-time error with GCC
set_property(SOURCE ${mpack_SOURCE_DIR}/src/mpack/mpack.c PROPERTY LANGUAGE CXX)
target_include_directories(mpack PUBLIC ${mpack_SOURCE_DIR}/src)

FetchContent_Declare(meshoptimizer
	GIT_REPOSITORY https://github.com/zeux/meshoptimizer
	GIT_TAG v0.18)
FetchContent_MakeAvailable(meshoptimizer)

FetchContent_Declare(incbin
	GIT_REPOSITORY https://github.com/graphitemaster/incbin
	GIT_TAG 6e576cae5ab5810f25e2631f2e0b80cbe7dc8cbf)
FetchContent_MakeAvailable(incbin)
add_library(incbin
	${incbin_SOURCE_DIR}/incbin.h ${incbin_SOURCE_DIR}/incbin.c)
target_compile_definitions(incbin PUBLIC INCBIN_PREFIX=g_)
target_compile_definitions(incbin PUBLIC INCBIN_STYLE=INCBIN_STYLE_SNAKE)
target_include_directories(incbin PUBLIC ${incbin_SOURCE_DIR})
