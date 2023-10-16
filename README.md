# Argus Game Engine ![Argus](https://github.com/caseif/Argus/workflows/Argus/badge.svg)

Argus is a 2D game engine written in C++17 and built atop SDL 2.

## Features

Argus features a modular architecture which allows subsystems to be enabled/disabled as needed. See the
[module system wiki page](https://github.com/caseif/Argus/wiki/Module-System) for more information on how this
architecture works.

Also see the [engine subsystems wiki page](https://github.com/caseif/Argus/wiki/Engine-Subsystems) for an overview of
the engine's architecture in its current iteration.

## Philosophy

### DIY

Argus was created as a hobby project with the goal of learning as much as possible. As such, it attempts to implement
as many of its features in-house as possible, with a couple of notable exceptions:

**Windowing/input polling:** For the moment, Argus uses SDL for OS-level "grunt" work including window and input
management. These tasks are highly OS-specific and thus would be somewhat tedious to maintain, so I would rather avoid
dedicating any time towards it at least until the project is in a more complete state.

**File format support:** The remaining dependencies are devoted to parsing file/data formats including PNG, JSON, and
DEFLATE data. I am not especially interested in this sort of task and do not see much educational value in it, and
developing [libarp](https://github.com/caseif/libarp) certainly gave me my fill of it anyhow.

### Platform Support

My vision is for Argus to support at least a handful of platforms, among these macOS, Android, and *BSD. This ties into
my goal of this being a learning project. This is lower priority though, and it currently only supports Windows and
Linux.

### Modularity

Argus is designed to be as modular as possible. A game engine's code base will by nature be very large and complex, and
implementing barriers between subsystems will (at least in theory) help it scale as more and more functionality is
implemented. This also helps to delineate internal and external dependencies of different subsystems very clearly, as
modules must explicitly specify which other modules and external libraries they require.

### Code/Architecture Quality

One of the main focuses for Argus has been the quality of the overall architecture and code. This has lead to a large
number of rewrites and refactors as I learn better ways to architect features and has slowed the project down quite a
bit, but the primary goal isn't necessarily to ship a game. Consequently, I've generally prioritized the quality of the
engine versus getting something full-featured out the door quickly.

## Building

Building Argus requires CMake and a relatively recent version of GCC, Clang, or MSVC with support for C++17 features.
The following software is required by the build process and must be installed and available on the path:

- Python 3
- Ruby 3
  - Bundler
- Rust (`cargo`)
- Vulkan SDK (if building the Vulkan backend)

Argus (the base library and respective render backend modules) depends on the following libraries:

- [SDL 2](https://github.com/libsdl-org/SDL)
- [glslang](https://github.com/KhronosGroup/glslang)
- [nlohmann/json](https://github.com/nlohmann/json)
- [libarp](https://github.com/caseif/libarp)
- [libpng](https://github.com/glennrp/libpng)
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross)
- [zlib](https://github.com/madler/zlib) (transitively through libpng)

Additionally, the following tools are required as part of the build script tooling:

- [Abacus](https://github.com/caseif/Abacus)
- [Aglet](https://github.com/caseif/Aglet)
- [arptool](https://github.com/caseif/arptool)

These library and tool dependencies are included as Git submodules and built/configured automatically by the build
script. The respective shared libraries (where applicable) will be generated as part of the distribution alongside
Argus's shared library.

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
