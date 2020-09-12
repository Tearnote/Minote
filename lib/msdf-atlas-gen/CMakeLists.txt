cmake_minimum_required(VERSION 3.15)
project(msdf-atlas-gen VERSION 1.0
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
add_compile_definitions(MSDFGEN_USE_CPP11)
add_compile_definitions(MSDF_ATLAS_STANDALONE)

find_package(PkgConfig REQUIRED)
pkg_search_module(FREETYPE REQUIRED freetype2)
include_directories(${FREETYPE_INCLUDE_DIRS})
find_package(Threads REQUIRED)

set(CXX_SOURCES
	msdf-atlas-gen/AtlasGenerator.h
	msdf-atlas-gen/AtlasStorage.h
	msdf-atlas-gen/BitmapAtlasStorage.h
	msdf-atlas-gen/BitmapAtlasStorage.hpp
	msdf-atlas-gen/bitmap-blit.cpp
	msdf-atlas-gen/bitmap-blit.h
	msdf-atlas-gen/Charset.cpp
	msdf-atlas-gen/Charset.h
	msdf-atlas-gen/charset-parser.cpp
	msdf-atlas-gen/csv-export.cpp
	msdf-atlas-gen/csv-export.h
	msdf-atlas-gen/DynamicAtlas.h
	msdf-atlas-gen/DynamicAtlas.hpp
	msdf-atlas-gen/GlyphBox.h
	msdf-atlas-gen/glyph-generators.cpp
	msdf-atlas-gen/glyph-generators.h
	msdf-atlas-gen/GlyphGeometry.cpp
	msdf-atlas-gen/GlyphGeometry.h
	msdf-atlas-gen/image-encode.cpp
	msdf-atlas-gen/image-encode.h
	msdf-atlas-gen/image-save.h
	msdf-atlas-gen/image-save.hpp
	msdf-atlas-gen/ImmediateAtlasGenerator.h
	msdf-atlas-gen/ImmediateAtlasGenerator.hpp
	msdf-atlas-gen/json-export.cpp
	msdf-atlas-gen/json-export.h
	msdf-atlas-gen/main.cpp
	msdf-atlas-gen/msdf-atlas-gen.h
	msdf-atlas-gen/Rectangle.h
	msdf-atlas-gen/RectanglePacker.cpp
	msdf-atlas-gen/RectanglePacker.h
	msdf-atlas-gen/rectangle-packing.h
	msdf-atlas-gen/rectangle-packing.hpp
	msdf-atlas-gen/Remap.h
	msdf-atlas-gen/size-selectors.cpp
	msdf-atlas-gen/size-selectors.h
	msdf-atlas-gen/TightAtlasPacker.cpp
	msdf-atlas-gen/TightAtlasPacker.h
	msdf-atlas-gen/types.h
	msdf-atlas-gen/utf8.cpp
	msdf-atlas-gen/utf8.h
	msdf-atlas-gen/Workload.cpp
	msdf-atlas-gen/Workload.h)

include_directories("msdfgen")
include_directories("msdfgen/include")
set(CXX_DEP_SOURCES
	msdfgen/core/arithmetics.hpp
	msdfgen/core/Bitmap.h
	msdfgen/core/Bitmap.hpp
	msdfgen/core/BitmapRef.hpp
	msdfgen/core/Contour.cpp
	msdfgen/core/Contour.h
	msdfgen/core/contour-combiners.cpp
	msdfgen/core/contour-combiners.h
	msdfgen/core/EdgeColor.h
	msdfgen/core/edge-coloring.cpp
	msdfgen/core/edge-coloring.h
	msdfgen/core/EdgeHolder.cpp
	msdfgen/core/EdgeHolder.h
	msdfgen/core/edge-segments.cpp
	msdfgen/core/edge-segments.h
	msdfgen/core/edge-selectors.cpp
	msdfgen/core/edge-selectors.h
	msdfgen/core/equation-solver.cpp
	msdfgen/core/equation-solver.h
	msdfgen/core/estimate-sdf-error.cpp
	msdfgen/core/estimate-sdf-error.h
	msdfgen/core/msdfgen.cpp
	msdfgen/core/pixel-conversion.hpp
	msdfgen/core/rasterization.cpp
	msdfgen/core/rasterization.h
	msdfgen/core/render-sdf.cpp
	msdfgen/core/render-sdf.h
	msdfgen/core/save-bmp.cpp
	msdfgen/core/save-bmp.h
	msdfgen/core/save-tiff.cpp
	msdfgen/core/save-tiff.h
	msdfgen/core/Scanline.cpp
	msdfgen/core/Scanline.h
	msdfgen/core/Shape.cpp
	msdfgen/core/Shape.h
	msdfgen/core/shape-description.cpp
	msdfgen/core/shape-description.h
	msdfgen/core/SignedDistance.cpp
	msdfgen/core/SignedDistance.h
	msdfgen/core/Vector2.cpp
	msdfgen/core/Vector2.h
	msdfgen/lib/lodepng.cpp
	msdfgen/lib/tinyxml2.cpp
	msdfgen/ext/import-font.cpp
	msdfgen/ext/import-font.h
	msdfgen/ext/import-svg.cpp
	msdfgen/ext/import-svg.h
	msdfgen/ext/save-png.cpp
	msdfgen/ext/save-png.h)

add_executable(msdf-atlas-gen ${CXX_SOURCES} ${CXX_DEP_SOURCES})
target_link_libraries(msdf-atlas-gen ${FREETYPE_STATIC_LIBRARIES} -lfreetype)
target_link_libraries(msdf-atlas-gen Threads::Threads)
