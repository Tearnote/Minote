# Minote
A repo for an upcoming puzzle action game. Not playable, currently on
git as a personal backup.

## Building
Tested working on Linux and Windows (MSYS2). Should be portable to any
platform with OpenGL 3.3 core profile and GCC/MinGW.

### External libraries
Requires [`GLFW`](https://www.glfw.org/) 3.3.

`msdf-atlasgen` requires
[`FreeType`](https://www.freetype.org/index.html) and
[`Boost`](https://www.boost.org/).

### Included libraries
[`glad`](https://glad.dav1d.de/) loader generated on 2019-05-06, in
Public Domain.

[`linmath.h`](https://github.com/datenwolf/linmath.h) retrieved on
2019-05-18, used under WTFPL.

[`PCG PRNG`](http://www.pcg-random.org/) retrieved on 2019-05-29, used
under Apache license.

[`tinyobjloader-c`](https://github.com/syoyo/tinyobjloader-c) retrieved
on 2019-08-22, used under MIT license.

[`msdf-atlasgen`](https://github.com/decimad/msdf-atlasgen) retrieved on
2019-08-29, used under MIT license.