# Source folder structure

## Major threads
Besides the main header, none of the headers can be included from any
other folder.
- `main`: Program entry point, main thread. Spawns the logic and render
  threads. Handles window events and user input.
- `logic`: Logic thread. Keeps a thread-local state, advances it
  and uploads changes to global state.
- `render`: Rendering thread. Once per frame it makes a thread-local
  copy of global state and draws it on the screen.

## Helper code
All code in this category must be thread-safe.
- `global`: Facilities for cross-thread communication.
- `types`: Structure definitions.
- `util`: Various useful facilities.

## Other
- `tools`: Executables used only during the build process.
- `glsl`: Shader sources.
- `obj`: Mesh data.