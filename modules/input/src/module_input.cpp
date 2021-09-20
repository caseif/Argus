/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/macros.hpp"

// module core
#include "argus/core/event.hpp"
#include "argus/core/module.hpp"

// module input
#include "internal/input/input_helpers.hpp"
#include "internal/input/module_input.hpp"

// module wm
#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"
#include "internal/wm/window.hpp"

#include "GLFW/glfw3.h"

#include <string>

namespace argus {
    static void _init_window_input(const Window &window) {
        init_keyboard(static_cast<GLFWwindow*>(get_window_handle(window)));
    }

    static void _on_window_event(const ArgusEvent &event, void *data) {
        UNUSED(data);
        auto &wevent = static_cast<const WindowEvent&>(event);
        if (wevent.subtype == WindowEventType::Create) {
            _init_window_input(wevent.window);
        }
    }

    void update_lifecycle_input(const LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Init:
                register_event_handler(ArgusEventType::Window, _on_window_event, TargetThread::Update);
                break;
            default:
                break;
        }
    }

    void init_module_input(void) {
    }

}
