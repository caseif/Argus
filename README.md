# Argus Game Engine ![Argus](https://github.com/caseif/Argus/workflows/Argus/badge.svg)

Argus is a 2D game engine written in C++14 and built atop GLFW.

## Features

Argus features a modular architecture which allows features to be enabled as needed. There are three types of modules
present in the engine at this point:

### Static Modules

A collection of built-in modules comprising the "core" of the engine. These modules are statically linked into a single
shared library and can be enabled as needed at runtime.

| Name | Description |
| --- | :-- |
| core | Core engine framework; facilitates basic operation and communication among components. |
| wm | Window manager. |
| ecs | Entity-component-system implementation. |
| resman | Resource manager; facilitates resource loading and lifetime management. |
| render | The base renderer. This performs tasks related to scene and resource management but does not provide a standalone renderer implementation and must be supplemented by a dynamic backend. |
| input | Input manager; responsible for interpreting keyboard, mouse, and controller input. This is dependent on the wm module because input polling is done with respect to a window. |

### Dynamic Modules

External modules which exist as plug-ins to the core of the engine. These modules are distributed as separate shared
libraries and are loaded dynamically at runtime by the engine core.

Render backends are distributed as dynamic modules to ease shipping them on platforms which may not necessarily support
the respsective graphics API.

| Name | Description |
| --- | :-- |
| render_opengl | An implementation of the renderer atop OpenGL. |

### Engine Libraries

Engine libraries are collections of common utilities which do not partake in the lifecycle of the engine and are instead
intended for use by other parts of the engine. They are statically linked but cannot be enabled or disabled at runtime.

| Name | Description |
| --- | :-- |
| lowlevel | Low-level platform-independence and utility code for features such as threading, memory management, and math. |

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

### Code/Architecture Quality

One of the main focuses for Argus has been the quality of the overall architecture as well as of the code comprising the
project. This has lead to a large number of rewrites and refactors and generally has slowed the project down greatly,
but because the primary goal is not necessarily to ship a game I consider this tradeoff justified.

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

Argus is made available under the [LGPLv3 license](https://opensource.org/licenses/LGPL-3.0). You may use, modify, and
distribute the project within its terms.

While modifications to static modules must be distributed under the same license, dynamically linked plug-in modules
need not be, nor do any resources (including scripts) used alongside the engine, whether on the filesystem or in
packaged form.
