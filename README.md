# Minote
An upcoming puzzle action game. Currently being rewritten in C++17.

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
[`gsl-lite`](https://github.com/martinmoene/gsl-lite) retrieved on 2019-12-07,
used under MIT license.

[`asap`](https://github.com/mobius3/asap) retrieved on 2019-12-08, used
under MIT license.

[`glad`](https://glad.dav1d.de/) loader generated on 2019-05-06, in
Public Domain.

[`SDL_GameControllerDB`](https://github.com/gabomdq/SDL_GameControllerDB)
retrieved on 2019-09-05, used under zlib license.
