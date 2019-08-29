# Minote
A repo for an upcoming puzzle action game. Not playable, currently on git as a personal backup.

## Building
Currently developed and frequently tested on Windows, but sometimes verified on Linux. Should be portable to any platform with OpenGL 3.3 core profile and some implementation of POSIX.

Built on Linux with CMake. Requires [GLFW](https://www.glfw.org/) 3.3, which is available as a package in any good distro. Also compiles for Windows via MinGW.

## External components
[GLFW](https://www.glfw.org/) 3.3, used under Zlib license.

[glad](https://glad.dav1d.de/) loader generated on 2019-05-06, in Public Domain.

[linmath.h](https://github.com/datenwolf/linmath.h) retrieved on 2019-05-18, used under WTFPL.

[PCG PRNG](http://www.pcg-random.org/) retrieved on 2019-05-29, used under Apache license.

[tinyobjloader-c](https://github.com/syoyo/tinyobjloader-c) retrieved on 2019-08-22, used under MIT license.

Please note that Minote's license is incompatible with GPL.