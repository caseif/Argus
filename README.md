# Argus Game Engine

Argus is a 2D game engine written in C++11 and built on OpenGL and SDL 2.

### Features

Argus features a modular engine architecture which allows features to be enabled
as needed. Furthermore, it offers great ease in expanding, as new modules can be
integrated into the engine with little effort. The current stock modules are as
follows:

| Layer | Name | Description |
| --- | --- | :-- |
| 0 | pi | Platform-independence code for features such as threading. |
| 1 | core | Core engine framework; facilitates basic operation and communication among components. |
| 2 | renderer | Renderer module; responsible for managing windows and rendering. |

A large number of additional modules are planned for future inclusion.

### Compiling

```
mkdir build
cd build
cmake ../
```

The appropriate build tools will be generated in the `build` directory you created.

### License

Argus is made available under the [MIT license](https://opensource.org/licenses/MIT). You may use, modify, and
distribute the project within its terms.
