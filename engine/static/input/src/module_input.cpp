/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#include "argus/lowlevel/macros.hpp"

#include "argus/core/event.hpp"
#include "argus/core/module.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"

#include "internal/input/controller.hpp"
#include "internal/input/gamepad.hpp"
#include "internal/input/input_manager.hpp"
#include "internal/input/keyboard.hpp"
#include "internal/input/module_input.hpp"
#include "internal/input/mouse.hpp"
#include "internal/input/script_bindings.hpp"

namespace argus {
    static void _init_window_input(const Window &window) {
        input::init_keyboard(window);
        input::init_mouse(window);
    }

    static void _on_window_event(const argus::WindowEvent &event, void *data) {
        UNUSED(data);

        if (event.subtype == argus::WindowEventType::Create) {
            _init_window_input(event.window);
        } else if (event.subtype == argus::WindowEventType::Focus) {
            //TODO: figure out how to move cursor inside window boundary
        }
    }


    static void _on_update_early(TimeDelta delta) {
        UNUSED(delta);
        input::ack_gamepad_disconnects();
    }

    static void _on_update_late(TimeDelta delta) {
        UNUSED(delta);
        input::flush_mouse_delta();
        input::flush_gamepad_deltas();
    }

    static void _on_render(TimeDelta delta) {
        UNUSED(delta);
        input::update_keyboard();
        input::update_mouse();
        input::update_gamepads();
    }

    extern "C" {
    void update_lifecycle_input(const argus::LifecycleStage stage) {
        switch (stage) {
            case argus::LifecycleStage::Init:
                register_update_callback(_on_update_early, Ordering::Early);
                register_update_callback(_on_update_late, Ordering::Late);
                register_render_callback(_on_render, Ordering::Early);
                register_event_handler<WindowEvent>(_on_window_event, argus::TargetThread::Render);

                register_input_script_bindings();

                break;
            case argus::LifecycleStage::Deinit:
                input::deinit_gamepads();

                break;
            default:
                break;
        }
    }
    }
}
