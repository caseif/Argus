// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "internal/wm/window.hpp"

// module input
#include "argus/input/mouse.hpp"

#include <GLFW/glfw3.h>

namespace argus {
    static Vector2d g_last_mouse_pos;

    void init_mouse(const Window &window) {
        auto glfw_handle = static_cast<GLFWwindow*>(get_window_handle(window));
        UNUSED(glfw_handle);
        //TODO
    }

    Vector2d get_mouse_pos(void) {
        //TODO
        return {};
    }

    Vector2d get_mouse_delta(void) {
        //TODO
        return {};
    }
}
