# Minote
An upcoming puzzle action game, written in C++20.

## Building
Continuously tested on Windows (MSYS2 MinGW), but should be portable to any
platform with OpenGL 3.3 core profile and a recent standards-compliant compiler.

Standard CMake build process:
```
mkdir build
cd build
cmake ..
cmake --build .
```

### External libraries
Requires:
- [`GLFW`](https://www.glfw.org/) 3.3
- [`HarfBuzz`](http://harfbuzz.org/)
- [`glm`](https://glm.g-truc.net/)
- [`fmt`](https://fmt.dev/)

### Included libraries
- [`glad`](https://glad.dav1d.de/) loader generated on 2020-10-10,
in Public Domain.
- [`PCG PRNG`](https://www.pcg-random.org/) Minimal C 0.9, used under Apache
license.
- [`SMAA (OpenGL port)`](https://github.com/turol/smaaDemo) retrieved
on 2020-02-29, used under MIT license.
[Original version.](https://www.iryoku.com/smaa/)
- [`Nuklear`](https://github.com/Immediate-Mode-UI/Nuklear) retrieved
on 2020-08-13, used under dual MIT license / Public Domain.
- [`Cephes Mathematical Library`](https://www.netlib.org/cephes) retrieved
on 2020-09-02, used under permissive terms.
- [`msdf-atlas-gen`](https://github.com/Chlumsky/msdf-atlas-gen) retrieved
on 2020-09-12, used under MIT license.
- [`stb_image.h`](https://github.com/nothings/stb/blob/master/stb_image.h)
retrieved on 2020-09-13, in Public Domain.
- [`scope_guard`](https://github.com/Neargye/scope_guard) retrieved
on 2020-10-05, used under MIT license.
- [`itlib`](https://github.com/iboB/itlib) retrieved on 2020-12-01, used under
MIT license.
- [`xassert`](https://github.com/rg3/xassert) retrieved on 2020-12-06, used
under CC0 license.
- [`robin-hood-hashing`](https://github.com/martinus/robin-hood-hashing)
retrieved on 2020-12-07, used under MIT license.
