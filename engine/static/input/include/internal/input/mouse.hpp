#pragma once

#include <GLFW/glfw3.h>

namespace argus {
    // forward declarations
    class Window;

    namespace input {
        void init_mouse(const argus::Window &window);
    }
}
