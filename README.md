# Argus Game Engine ![Argus](https://github.com/caseif/Argus/workflows/Argus/badge.svg)

Argus is a 2D game engine written in C++11 and built atop GLFW.

## Features

Argus features a modular architecture which allows features to be enabled as needed. The engine also provides support
for loading additional modules (distributed as shared libraries) as plug-ins at runtime automatically and dynamically.

| Layer | Name | Description |
| --- | --- | :-- |
| 0 | lowlevel | Low-level platform-independence code for features such as threading. Strictly speaking, this is not an engine "module," but rather a collection of support libraries. |
| 1 | core | Core engine framework; facilitates basic operation and communication among components. |
| 2 | wm | Window manager. |
| 2 | ecs | Entity-component-system implementation. |
| 2 | resman | Resource manager; facilitates resource loading and lifetime management. |
| 3 | render | Renderer module. |
| 3 | input | Input manager; responsible for interpreting keyboard, mouse, and controller input. This is dependent on the wm module because input polling is done with respect to a window. |

A large number of additional modules are planned for future inclusion.

## Philosophy

### DIY

Argus was created as a hobby project with the goal of learning as much as possible. As such, it attempts to implement
as many of its features in-house as possible, with a couple of notable exceptions:

**Windowing/input polling:** For the moment, Argus uses GLFW for OS-level "grunt" work including window and input
management. These tasks are highly OS-specific and thus would be somewhat tedious to implement, so I would rather
avoid dedicating any time towards it (if ever) until the project is in a more complete state.

**File format support:** The remaining dependencies are devoted to parsing file/data formats including PNG, JSON, and
DEFLATE data. I am not especially interested in this sort of task and do not see much educational value in it, and
developing [libarp](https://github.com/caseif/libarp) certainly gave me my fill of it anyhow.

### Platform Support

My vision is for Argus to support at least a handful of platforms, among these macOS, Android, and *BSD. This ties into
my goal of this being a learning project. This is lower priority though, and currently it only supports Windows and
Linux.

### Modularity

Argus is designed to be as modular as possible. A game engine by nature will possess a very large code base, and my
belief is that implementing barriers between functional areas will help it scale as more and more functionality is
added. This also helps to delineate internal and external dependencies of different parts of the code base very clearly,
as modules must explicitly specify which other modules and external libraries they require.

## Compiling

The base Argus library depends on the following libraries:

- [GLFW](https://github.com/glfw/glfw/)
- [nlohmann/json](https://github.com/nlohmann/json)
- [libarp](https://github.com/caseif/libarp/)
- [libpng](https://github.com/glennrp/libpng)
- [zlib](https://github.com/madler/zlib) (transitively through libpng)

Additionally, the following projects are required as part of the build script tooling:

- [Abacus](https://github.com/caseif/Abacus)
- [Aglet](https://github.com/caseif/Aglet)
- [arptool](https://github.com/caseif/arptool)

These dependencies are included as Git submodules and built/configured automatically by the build script. The
respective shared libraries (where applicable) will be generated as part of the distribution alongside Argus's shared
library.

Additionally, the render backends require support from the OS for their respective graphics libraries. Argus currently
provides an OpenGL-based backend, with OpenGL ES- and Vulkan-based backends planned for future inclusion.

To set up the build files, please run the following commands:

```bash
git submodule update --init --recursive
mkdir build
cd build
cmake ..
```

The appropriate build files will be generated in the `build` directory you created. You may use the generated tools
directly or run `cmake --build .`.

## License

Argus is made available under the [MIT license](https://opensource.org/licenses/MIT). You may use, modify, and
distribute the project within its terms.
