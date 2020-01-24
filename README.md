# Argus Game Engine

Argus is a 2D game engine written in C++11 and built on OpenGL/OpenGLES and SDL 2.

### Features

Argus features a modular architecture which allows features to be enabled as
needed. The modular engine is also highly extensible, as new modules can be
distributed alongside the base library and loaded at runtime automatically.

| Layer | Name | Description |
| --- | --- | :-- |
| 0 | lowlevel | Low-level platform-independence code for features such as threading. Strictly speaking, this is not an engine "module," but rather a collection of libraries. |
| 1 | core | Core engine framework; facilitates basic operation and communication among components. |
| 2 | resman | Resource manager; facilitates resource loading and lifetime management. |
| 3 | renderer | Renderer module; responsible for managing windows and rendering. |
| 4 | input | Input manager; responsible for interpreting keyboard, mouse, and controller input. |

A large number of additional modules are planned for future inclusion.

### Compiling

Argus depends on SDL 2, OpenGL (or alternatively OpenGLES), and libpng. These libraries must be present when building.
The build script will attempt to locate them automatically.

To set up the build files, please run the following commands:

```bash
mkdir build
cd build
cmake ../
```

The appropriate build files will be generated in the `build` directory you
created.

### License

Argus is made available under the [MIT license](https://opensource.org/licenses/MIT). You may use, modify, and
distribute the project within its terms.
