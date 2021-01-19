/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module core
#include "argus/core.hpp"

// module input
#include "argus/input.hpp"
#include "internal/input/input_helpers.hpp"

// module render
#include "argus/render/window.hpp"
#include "argus/render/window_event.hpp"
#include "internal/render/window.hpp"

#include "GLFW/glfw3.h"

namespace argus {
    static void _init_window_input(const Window &window) {
        init_keyboard(static_cast<GLFWwindow*>(get_window_handle(window)));
    }

    static void _on_window_event(const ArgusEvent &event, void *data) {
        const WindowEvent wevent = static_cast<const WindowEvent&>(event);
        if (wevent.subtype == WindowEventType::CREATE) {
            _init_window_input(wevent.window);
        }
    }

    void _update_lifecycle_input(const LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::INIT:
                register_event_handler(ArgusEventType::WINDOW, _on_window_event, TargetThread::UPDATE);
                break;
            default:
                break;
        }
    }

    void init_module_input(void) {
        register_module({MODULE_INPUT, 4, {"core"}, _update_lifecycle_input});
    }

}
