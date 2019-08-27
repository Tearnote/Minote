# Minote
A repo for an upcoming puzzle action game. Not playable, currently on git as a personal backup.

## Building
Currently developed and frequently tested on Windows, but sometimes verified on Linux. Should be portable to any platform with OpenGL 3.3 core profile and some implementation of POSIX.

Built on Linux with CMake. Requires [GLFW](https://www.glfw.org/) 3.3, which is available as a package in any good distro. Also compiles for Windows via MinGW.

## External components
[GLFW](https://www.glfw.org/), licensed under [Zlib license](https://opensource.org/licenses/zlib-license.php).

[glad](https://glad.dav1d.de/) generated loader, in Public Domain.

[linmath.h](https://github.com/datenwolf/linmath.h), licensed under [WTFPL](http://www.wtfpl.net/).

[winpthreads](https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-libraries/winpthreads/), licensed under [MIT license](https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-libraries/winpthreads/COPYING).

[MinGW runtime](http://mingw-w64.org/doku.php/start) licensed under a mix of licenses included in the repository as `LICENSE.MinGW-w64.runtime.txt`, the least permissive of which being LGPL.

[PCG PRNG](http://www.pcg-random.org/) licensed under Apache license, included in the repository as `LICENSE.PCG.txt`.

[tinyobjloader-c](https://github.com/syoyo/tinyobjloader-c) licensed under MIT license.

Please note that Minote's license is incompatible with GPL.