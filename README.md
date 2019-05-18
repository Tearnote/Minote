# Minote
A repo for an upcoming rhythm puzzle action game. Not playable, currently only on git as a personal backup.

## Building
Currently only developed and tested on Windows, but should be portable to any platform with OpenGL and an implementation of POSIX. Install cygwin64, inside it install mingw-w64. Requires glfw. Uses MinGW's winpthreads on Windows, requires `libwinpthread-1.dll` at runtime.

Run `make MINGW=1` for a release build or `make MINGW=1 DEBUG=1` for a debug build. In addition to debug symbols they also differ in logging verbosity and sanity checking. Debug build requires `libssp-0.dll` at runtime for stack protection.