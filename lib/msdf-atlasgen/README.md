# msdf-atlasgen
Create MSDF texture atlas and font description from TrueType fonts

Command-line tool to create Multichannel Signed Distance Field texture atlas and font description from TrueType fonts.

Visual Studio Project - for compilation set the correct paths in boost-1.61.props

Call with "--help" to see arguments.

## Compile with CMake

To compile with cmake, just run the usual:

    mkdir build && cd build
    cmake ..
    make
    
Freetype and Boost are required.
