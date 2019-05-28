# Minote
A repo for an upcoming rhythm puzzle action game. Not playable, currently on git as a personal backup.

## Building
Currently developed and frequently tested on Windows, but sometimes verified on Linux. Should be portable to any platform with OpenGL 3.3 core profile and some implementation of POSIX.

Currently being built with [mingw-w64](https://mingw-w64.org/doku.php/download/cygwin) inside a cygwin install, the Makefile requires `xxd` to preprocess shader sources and might expand to more Unix tools in the future. Requires [GLFW](https://www.glfw.org/).

Run `make MINGW=1` for a release build or `make MINGW=1 DEBUG=1` for a debug build. In addition to debugging symbols, the debug build also features higher logging verbosity and runtime memory protection.

On Windows the executable requires `libwinpthread-1.dll` for pthread support, additionally the debug build requires `libssp-0.dll` for stack protection. You can find these inside your mingw-w64.

## External components
[GLFW](https://www.glfw.org/), licensed under [Zlib license](https://opensource.org/licenses/zlib-license.php).

[glad](https://glad.dav1d.de/) generated loader, in Public Domain.

[linmath.h](https://github.com/datenwolf/linmath.h), licensed under [WTFPL](http://www.wtfpl.net/).

[winpthreads](https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-libraries/winpthreads/), licensed under [MIT license](https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-libraries/winpthreads/COPYING).

[MinGW runtime](http://mingw-w64.org/doku.php/start) licensed under a mix of licenses included in the repository as `LICENSE.MinGW-w64.runtime.txt`, the least permissive of which being LGPL.

Please note that Minote's license is incompatible with GPL.