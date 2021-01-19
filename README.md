# Argus Game Engine ![Argus](https://github.com/caseif/Argus/workflows/Argus/badge.svg)

Argus is a 2D game engine written in C++11 and built on GLFW.

### Features

Argus features a modular architecture which allows features to be enabled as needed. The engine also provides support
for loading additional modules (distributed as shared libraries) at runtime automatically and dynamically.

| Layer | Name | Description |
| --- | --- | :-- |
| 0 | lowlevel | Low-level platform-independence code for features such as threading. Strictly speaking, this is not an engine "module," but rather a collection of support libraries. |
| 1 | core | Core engine framework; facilitates basic operation and communication among components. |
| 2 | wm | Window manager. |
| 2 | ecs | Entity-component-system implementation. |
| 2 | resman | Resource manager; facilitates resource loading and lifetime management. |
| 3 | render | Renderer module. |
| 3 | input | Input manager; responsible for interpreting keyboard, mouse, and controller input. This is dependent on the render module because input reading is done respective to a window. |

A large number of additional modules are planned for future inclusion.

### Compiling

The base Argus library depends on GLFW and libpng. These libraries are included as Git submodules and are built
automatically by the build script. The appropriate shared libraries will be generated as part of the distribution
alongside Argus's shared library.

Additionally, the render backends require support from the OS for their respective graphics libraries. Argus currently
provides an OpenGL-based backend, with OpenGL ES- and Vulkan-based backends planned for future inclusion.

To set up the build files, please run the following commands:

```bash
git submodule update --init --recursive
mkdir build
cd build
cmake ..
```

The appropriate build files will be generated in the `build` directory you
created. You may use the generated tools directly or run `cmake --build .`.

### License

Argus is made available under the [MIT license](https://opensource.org/licenses/MIT). You may use, modify, and
distribute the project within its terms.
