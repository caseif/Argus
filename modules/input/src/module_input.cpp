/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
