# Minote
An upcoming puzzle action game. Rather barebones right now, but fully
playable.

## Usage
`Minote` - Start new game, overwrite `replay.mre` on close

`Minote --help` - Print usage help

`Minote --replay` - Open `replay.mre` in replay viewer

`Minote --fullscreen` - Use exclusive fullscreen mode

`Minote --nosync` - Disable hard GPU sync for higher performance at the
cost of latency

## Building
Tested working on Linux and Windows (MSYS2). Should be portable to any
platform with OpenGL 3.3 core profile and GCC/MinGW.

Standard CMake build process:
```
mkdir build
cd build
cmake ..
make
```

### External libraries
Requires [`GLFW`](https://www.glfw.org/) 3.3,
[`libunistring`](https://www.gnu.org/software/libunistring/) and
[`LibLZMA`](https://tukaani.org/xz/).

### Included libraries
[`glad`](https://glad.dav1d.de/) loader generated on 2019-05-06, in
Public Domain.

[`linmath.h`](https://github.com/datenwolf/linmath.h) retrieved on
2019-05-18, used under WTFPL.

[`PCG PRNG`](http://www.pcg-random.org/) retrieved on 2019-05-29, used
under Apache license.

[`tinyobjloader-c`](https://github.com/syoyo/tinyobjloader-c) retrieved
on 2019-08-22, used under MIT license.

[`stb_image.h`](https://github.com/nothings/stb) retrieved on
2019-08-29, in Public Domain.

[`SDL_GameControllerDB`](https://github.com/gabomdq/SDL_GameControllerDB)
retrieved on 2019-09-05, used under zlib license.

[`parson`](https://github.com/kgabis/parson) retrieved on 2019-09-06,
used under MIT license.