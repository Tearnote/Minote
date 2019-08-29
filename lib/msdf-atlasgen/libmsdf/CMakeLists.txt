include_directories(${FREETYPE_INCLUDE_DIRS} "include")

add_library(lodepng STATIC "lib/lodepng.cpp")
add_library(tinyxml2 STATIC "lib/tinyxml2.cpp")

add_library(msdf STATIC
  "core/Bitmap.cpp"
  "core/Contour.cpp"
  "core/edge-coloring.cpp"
  "core/edge-segments.cpp"
  "core/EdgeHolder.cpp"
  "core/equation-solver.cpp"
  "core/msdfgen.cpp"
  "core/render-sdf.cpp"
  "core/save-bmp.cpp"
  "core/shape-description.cpp"
  "core/Shape.cpp"
  "core/SignedDistance.cpp"
  "core/Vector2.cpp"
  "ext/import-font.cpp"
  "ext/import-svg.cpp"
  "ext/save-png.cpp"
  )
add_dependencies(msdf lodepng tinyxml2)

target_link_libraries(msdf ${FREETYPE_LIBRARIES} lodepng tinyxml2)
