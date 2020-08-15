# Minote
An upcoming puzzle action game, written in C11.

## Building
Continuously tested on Linux (GCC) and Windows (MSYS2 MinGW). Should be
portable to any platform with OpenGL 3.3 core profile and any
standards-compliant compiler.

Standard CMake build process:
```
mkdir build
cd build
cmake ..
cmake --build .
```

### External libraries
Requires [`GLFW`](https://www.glfw.org/) 3.3.

### Included libraries
[`glad`](https://glad.dav1d.de/) loader generated on 2019-05-06, in
Public Domain.

[`linmath.h`](https://github.com/datenwolf/linmath.h) retrieved on
2020-01-01, used under WTFPL.

[`SDL_GameControllerDB`](https://github.com/gabomdq/SDL_GameControllerDB)
retrieved on 2019-09-05, used under zlib license.

[`PCG PRNG`](https://www.pcg-random.org/) Minimal C 0.9, used under Apache
license.

[`AHEasing`](https://github.com/warrenm/AHEasing) retrieved on 2020-02-21, used
under WTFPL.

[`SMAA (OpenGL port)`](https://github.com/turol/smaaDemo) retrieved on
2020-02-29, used under MIT license.
[Original version.](https://www.iryoku.com/smaa/)

[`Nuklear`](https://github.com/Immediate-Mode-UI/Nuklear) retrieved on
2020-08-13, used under dual MIT license / Public Domain.
