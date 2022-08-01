# Minote
A work-in-progress experimental hobbyist renderer for personal videogame and
digital art projects.

More details about current state and future plans on
[Trello](https://trello.com/b/LsFEM6Vw/minote)!

## Goals
- Performant rendering of models with high instance counts
- Artifact-free shading and post-processing, approaching path-tracing quality
when possible
- Flexibility to rapidly iterate on interesting rendering techniques
- Compatibility with most of the modern gaming-grade hardware
- Portable code, open-source libraries and toolchain

## Non-goals
- Texture mapping
- Vertex animation
- Being an engine

## Building
Developed on Windows in a MSYS2 UCRT environment. Requires CMake and the Vulkan
SDK installed; no other external dependencies. Standard CMake build process,
the Ninja Multi-Config generator is recommended.

## Libraries used
- [`volk`](https://github.com/zeux/volk) (MIT)
- [`vuk`](https://github.com/martty/vuk) (MIT)
- [`vk-bootstrap`](https://github.com/charles-lunarg/vk-bootstrap) (MIT)
- [`PCG`](https://github.com/imneme/pcg-c-basic) (Apache-2.0)
- [`SDL`](https://github.com/libsdl-org/SDL) (Zlib)
- [`Dear ImGui`](https://github.com/ocornut/imgui) (MIT)
- [`SQLite`](https://www.sqlite.org/) (Public Domain)
- [`cgltf`](https://github.com/jkuhlmann/cgltf) (MIT)
- [`robin_hood`](https://github.com/martinus/robin-hood-hashing) (MIT)
- [`itlib`](https://github.com/iboB/itlib) (MIT)
- [`{fmt}`](https://github.com/fmtlib/fmt) (MIT)
- [`Quill`](https://github.com/odygrd/quill) (MIT)
- [`GCE-Math`](https://github.com/kthohr/gcem) (Apache-2.0)
- [`MPack`](https://github.com/ludocode/mpack) (MIT)
- [`meshoptimizer`](https://github.com/zeux/meshoptimizer) (MIT)
- [`bvh`](https://github.com/madmann91/bvh) (MIT)
- [`ìncbin`](https://github.com/graphitemaster/incbin) (Unlicense)
- Smaller code snippets attributed inline
