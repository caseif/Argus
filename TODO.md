# TODO List

This is a non-exhaustive list of tasks that need to be completed in order to bring Argus into a usable state.

This list was last updated on 2021/02/19.

### Resource management

Resource management is being overhauled to support the use of both [ARP](https://github.com/caseif/libarp) packages and
in-memory resources.

- Complete [libarp](https://github.com/caseif/libarp) (ETA 2-4 weeks)
- Write arptool (libarp wrapper) (ETA 1-2 weeks)
- Integrate libarp into Argus (ETA quick)
- Add events for resource loading/unloading (ETA quick)
- Make shaders/materials resources (ETA quick)
- Perform shader/material deinitialization in event handlers instead of through direct invocation (ETA quick)
- Pre-pack required resources as in-memory ARP in binary (ETA 1 week)

### Renderer

The renderer needs some light refactoring, and additional graphical backends are planned. This is non-blocking.

- Generate `glfuncs.h` on CMake configure (ETA 1 week)
- Add built-in post-processing effects (ETA 1-2 weeks)
- Add functions for getting available display capabilities (resolution, refresh rate, etc.)
- Write OpenGL ES backend (`render_opengl_es`) (ETA 2-4 weeks)
- Write Vulkan backend (`render_vulkan`) (ETA 2-3 months)

### Input

The `input` module needs a significant rewrite to abstract away lower-level input processing.

- Rewrite `input` module (ETA 2-4 weeks)

### ECS

The ECS module was never completed and needs some additional work to bring it into a usable state.

- Finish `ecs` module (ETA 2-4 weeks)

### Sound

A module for managing sound will be needed.

- Write `sound` module (ETA 1-2 months)

### Scripting

A scripting engine is required to provide an alternative to implementing game logic as a plug-in module. The engine
will most likely use Lua as its language. A stretch goal is to design and implement a custom scripting language.

- Write scripting module (`scripting`) (ETA 2-3 months)
- Possibly write custom scripting language (far-future) (ETA ???)

### UI

A module is required for providing support for defining in-game menus in an abstract way.

- Write `ui` module (ETA 1-2 months)

### Simulation

A simulation engine is required to provide abstract models corresponding to the game world.

- Write simulation module(s) (ETA ???)
- Design/select level format (ETA ???)
- Design/select level editor (ETA ???)

### Editor

An editor would be good to have for making the processing of game development more streamlined.

- Write editor (ETA loooong)

### Misc

Some work is needed on the in-line documentation.

- Global documentation pass (ETA 1-2 weeks)
