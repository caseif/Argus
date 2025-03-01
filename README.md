# Argus Game Engine ![Argus](https://github.com/caseif/Argus/workflows/Argus/badge.svg)

Argus is a 2D game engine written in Rust and built atop SDL 2.

## Features

Argus features a modular architecture which allows subsystems to be enabled/disabled as needed. See the
[module system wiki page](https://github.com/caseif/Argus/wiki/Module-System) for more information on how this
architecture works.

Also see the [engine subsystems wiki page](https://github.com/caseif/Argus/wiki/Engine-Subsystems) for an overview of
the engine's architecture in its current iteration.

## Philosophy

### Modularity

Argus is designed to be as modular as possible. A game engine's code base will by nature be very large and complex, and
implementing barriers between subsystems will (at least in theory) help it scale as more and more functionality is
implemented. This also helps to delineate internal and external dependencies of different subsystems very clearly, as
modules must explicitly specify which other modules and external libraries they require.

### Platform Support

My vision is for Argus to support at least a handful of platforms including macOS, Android, and *BSD. This ties into
my goal of this being a learning project. This is lower priority though, and the engine currently only supports Linux
and Windows (and at least builds on macOS).

### Code/Architecture Quality

One of the main focuses for Argus has been the quality of the overall architecture and code. This has lead to a large
number of rewrites and refactors as I learn better ways to architect features and has slowed the project down quite a
bit, but because the primary goal isn't necessarily to ship a game, I've generally prioritized code and architectural
quality over more quickly getting something full-featured out the door.

## Building

### Libraries

Building Argus requires Cargo and a relatively recent version of GCC, Clang, or MSVC with support for C++17 features.
The following additional software is required by the build process and must be installed and available on the path:

- Ruby 3, RubyGems, and Bundler
- Vulkan SDK if building the Vulkan backend, plus Vulkan validation layers for debug builds

Argus (the base library and respective render backend modules) depends on the following system libraries:

- [SDL 2](https://github.com/libsdl-org/SDL) >= 2.0.10
- [Lua](https://github.com/lua/lua) >= 5.4

### Tooling

Additionally, the following tools are required as part of the build script tooling:

- [Aglet](https://github.com/caseif/Aglet)

These tool dependencies are included as Git submodules and used automatically by the build script.

Additionally, the render backends require support from the OS for their respective graphics libraries. Argus currently
provides an OpenGL-based backend with OpenGL ES and Vulkan backends still in the process of being ported to Rust.

To build Argus, please run the following commands:

```bash
git submodule update --init --recursive
cargo build --features=argus/opengl
```

## License

Argus is made available under the [LGPLv3](https://opensource.org/licenses/LGPL-3.0). You may use, modify, and
distribute the project within its terms.
