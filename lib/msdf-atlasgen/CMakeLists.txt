cmake_minimum_required(VERSION 2.8)

project(msdf-atlasgen)

find_package(Freetype REQUIRED)
find_package(Boost REQUIRED
  program_options )

add_subdirectory("libmsdf")

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
  "libmsdf/include"
  "libmsdf/ext"
)

add_executable(msdf-atlasgen "msdf-atlasgen/main.cpp")
add_dependencies(msdf-atlasgen msdf)
target_link_libraries(msdf-atlasgen
  ${Boost_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  msdf
)
