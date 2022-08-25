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
- Modern and self-contained C++ codebase

## Non-goals
- Texture mapping
- Vertex animation
- Being an engine
- Cross-platform support

## Building
Open in Visual Studio 2022 as a CMake project and build with MSVC. Requires
the Vulkan SDK to be installed; no other dependencies. `Debug` and `Release`
configurations are self-explanatory, `RelWithDebInfo` is designed for profiling.

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
- [`fmtlog`](https://github.com/MengRao/fmtlog) (MIT)
- [`GCE-Math`](https://github.com/kthohr/gcem) (Apache-2.0)
- [`MPack`](https://github.com/ludocode/mpack) (MIT)
- [`meshoptimizer`](https://github.com/zeux/meshoptimizer) (MIT)
- [`ìncbin`](https://github.com/graphitemaster/incbin) (Unlicense)
- [`SPD`](https://github.com/GPUOpen-Effects/FidelityFX-SPD) (MIT)
- Smaller code snippets attributed inline
