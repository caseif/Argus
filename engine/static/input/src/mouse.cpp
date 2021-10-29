// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/math.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "internal/wm/window.hpp"

// module input
#include "argus/input/mouse.hpp"

#include <GLFW/glfw3.h>

namespace argus::input {
    static argus::Vector2d g_last_mouse_pos;

    void init_mouse(const argus::Window &window) {
        auto glfw_handle = static_cast<GLFWwindow*>(argus::get_window_handle(window));
        UNUSED(glfw_handle);
        //TODO
    }

    argus::Vector2d mouse_position(const argus::Window &window) {
        //TODO
        UNUSED(window);
        return {};
    }

    argus::Vector2d mouse_delta(const argus::Window &window) {
        //TODO
        UNUSED(window);
        return {};
    }
}
