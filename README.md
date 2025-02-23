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

Argus was created as a hobby project with the goal of learning as much as possible. It attempts to implement as much
functionality in-house as possible, with some notable exceptions:

**Windowing/input polling:** For the moment, Argus uses SDL for OS-level "grunt" work including window and input
management. These tasks are highly OS-specific and would be somewhat tedious to maintain, so I would rather avoid
dedicating any time towards it (at least until the project is much further along).

**File format support:** The remaining dependencies are devoted to parsing file/data formats including PNG, JSON, and
DEFLATE data. Reimplementing this would be a pretty serious time investment, so I feel a "shortcut" is justified here.
In any event, developing [libarp](https://github.com/caseif/libarp) more than gave me my fill of parsing work.

### Platform Support

My vision is for Argus to support at least a handful of platforms including macOS, Android, and *BSD. This ties into
my goal of this being a learning project. This is lower priority though, and the engine currently only supports Linux
and Windows.

### Modularity

Argus is designed to be as modular as possible. A game engine's code base will by nature be very large and complex, and
implementing barriers between subsystems will (at least in theory) help it scale as more and more functionality is
implemented. This also helps to delineate internal and external dependencies of different subsystems very clearly, as
modules must explicitly specify which other modules and external libraries they require.

### Code/Architecture Quality

One of the main focuses for Argus has been the quality of the overall architecture and code. This has lead to a large
number of rewrites and refactors as I learn better ways to architect features and has slowed the project down quite a
bit, but because the primary goal isn't necessarily to ship a game, I've generally prioritized code and architectural
quality over more quickly getting something full-featured out the door.

## Building

### Libraries

Building Argus requires CMake and a relatively recent version of GCC, Clang, or MSVC with support for C++17 features.
The following software is required by the build process and must be installed and available on the path:

- Python 3
- Ruby 3, RubyGems, and Bundler
- Rust (`cargo`)
- Vulkan SDK if building the Vulkan backend, plus Vulkan validation layers for debug builds

Argus (the base library and respective render backend modules) depends on the following libraries:

- [libpng](https://github.com/glennrp/libpng) >= 1.6.37
- [zlib](https://github.com/madler/zlib) >= 1.2.11 (transitively through libpng)
- [SDL 2](https://github.com/libsdl-org/SDL) >= 2.0.10
- [nlohmann/json](https://github.com/nlohmann/json) >= 3.8
- [glslang](https://github.com/KhronosGroup/glslang) >= 13.1.0, <= 14.1.0
- [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools)
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross)
- [libarp](https://github.com/caseif/libarp)

The build script will attempt to use a system installation of all of these libraries on Linux and macOS, except for
libarp. This behavior can be overridden via the `NO_SYSTEM_LIBS` CMake flag as well as individual `USE_SYSTEM_<LIBRARY>`
flags for each respective library.

glslang, SPIRV-Tools, and SPIRV-Cross are relatively tightly coupled and the build script will always either use system
libraries for both or build both locally. All three libraries are controlled via the `USE_SYSTEM_GLSLANG` flag. The
build script will also automatically fall back to building them locally if adequate system installations can't be
found.

A system installation of `libuuid` is also required for Linux builds.

### Test Libraries

A relatively recent version of Catch2 is required for building and running tests. This will be built locally from a
submodule if a system installation is not detected.

Tests can be disabled via the `ARGUS_SKIP_TESTS` CMake flag.

### Tooling

Additionally, the following tools are required as part of the build script tooling:

- [Abacus](https://github.com/caseif/Abacus)
- [Aglet](https://github.com/caseif/Aglet)

These library and tool dependencies are included as Git submodules and built/configured automatically by the build
script. The respective shared libraries (where applicable) will be generated as part of the distribution alongside
Argus's shared library.

Additionally, the render backends require support from the OS for their respective graphics libraries. Argus currently
provides OpenGL, OpenGL ES, and Vulkan-based backends.

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
