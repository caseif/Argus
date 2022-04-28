# Argus Game Engine ![Argus](https://github.com/caseif/Argus/workflows/Argus/badge.svg)

Argus is a 2D game engine written in C++14 and built atop GLFW.

## Features

Argus features a modular architecture which allows features to be enabled as needed. See the
[module system wiki page](https://github.com/caseif/Argus/wiki/Module-System) for more information on how this
architecture works.

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
provides OpenGL-based and OpenGL ES-based backends, and a Vulkan backend is currently under development.

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

Argus is made available under the [LGPLv3](https://opensource.org/licenses/LGPL-3.0). You may use, modify, and
distribute the project within its terms.

Argus employs a modular architecture and makes a distinction between library, static, and dynamic modules. Library and
static modules are directly compiled into the "core" engine library (libargus.so, argus.dll, etc.), while dynamic
modules are compiled into individual shared libraries. Because the LGPL provides a safe harbor for dynamically linked
libraries, this distinction affects how modifications and additions to the engine must be licensed.

Modifications to any modules contained by this repository, including dynamic modules, must be published under the LGPL.
This includes resources which will be compiled into the target binary, via Aglet or any other means. No safe harbor
applies because dynamic modules in this repository are licensed under the LGPL.

Newly created library or static modules which are not contained by this repository must also be published under the
LGPL, because they will be compiled into a single shared library which inherits the LGPL licensing. This again includes
resources which will be compiled into the shared library.

Newly created dynamic modules which are not contained by this repository fall under the safe harbor and do not need to
be published under the LGPL.

Newly created resources which are not compiled into any binary and are instead dynamically loaded at runtime also do not
need to be published under the LGPL.
