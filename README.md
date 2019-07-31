# Minote
A repo for an upcoming rhythm puzzle action game. Not playable, currently on git as a personal backup.

## Building
Currently developed and frequently tested on Windows, but sometimes verified on Linux. Should be portable to any platform with OpenGL 3.3 core profile and some implementation of POSIX.

Currently being built with CMake in an [MSYS2](http://www.msys2.org/) environment.
Requires [GLFW](https://www.glfw.org/), which is available as an MSYS2 package.
Requires [FMOD](https://fmod.com/) 1.10.x. On Windows install the FMOD Studio API on your system, then copy `"C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\lowlevel\lib\fmod64_vc.lib"` and `fmodL64_vc.lib` to your `lib` directory as `fmod.lib` and `fmodL.lib`, the "L" version including extra debugging features.

On Windows the executable requires `libwinpthread-1.dll` for pthread support, you can find it inside your mingw files.
Also requires `fmod64.dll` for the release build or `fmodL64.dll` for the debug build, found in the same location as FMOD's `.lib` files.

## External components
[GLFW](https://www.glfw.org/), licensed under [Zlib license](https://opensource.org/licenses/zlib-license.php).

[glad](https://glad.dav1d.de/) generated loader, in Public Domain.

[linmath.h](https://github.com/datenwolf/linmath.h), licensed under [WTFPL](http://www.wtfpl.net/).

[winpthreads](https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-libraries/winpthreads/), licensed under [MIT license](https://sourceforge.net/p/mingw-w64/mingw-w64/ci/master/tree/mingw-w64-libraries/winpthreads/COPYING).

[MinGW runtime](http://mingw-w64.org/doku.php/start) licensed under a mix of licenses included in the repository as `LICENSE.MinGW-w64.runtime.txt`, the least permissive of which being LGPL.

[PCG PRNG](http://www.pcg-random.org/) licensed under Apache license, included in the repository as `LICENSE.PCG.txt`.

[FMOD Core](https://fmod.com/) licensed under a proprietary EULA included in the repository as `LICENSE.FMOD.txt`. The project falls under the <500k budget tier, granting it a free license. The end product will be required to include the FMOD logo on a splash screen.

Please note that Minote's license is incompatible with GPL.