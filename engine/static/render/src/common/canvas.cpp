/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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
#include "argus/lowlevel/time.hpp"
#include "argus/lowlevel/vector.hpp"

#include "argus/wm/window.hpp"
#include "argus/wm/window_event.hpp"

#include "argus/render/common/canvas.hpp"
#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/pimpl/common/canvas.hpp"
#include "internal/render/pimpl/common/scene.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace argus {
    // forward declarations
    struct ArgusEvent;

    Canvas::Canvas(Window &window):
            pimpl(new pimpl_Canvas(window)) {
    }

    Canvas::~Canvas() {
        delete pimpl;
    }

    Window &Canvas::get_window(void) const {
        return pimpl->window;
    }

    std::optional<Viewport2D> Canvas::find_viewport(const std::string &id) {
        auto it = pimpl->viewports.find(id);
        if (it == pimpl->viewports.end()) {
            throw std::runtime_error("Viewport with provided ID does not exist on the current camera");
        }
        return it->second.viewport;
    }

    void Canvas::attach_viewport_2d(const std::string &id, const Camera2D &camera, const Viewport2D &viewport) {
        if (pimpl->viewports.find(id) != pimpl->viewports.end()) {
            throw std::runtime_error("Viewport with provided ID already exists on the current camera");
        }

        pimpl->viewports.insert({ id, AttachedViewport(camera, viewport) });
    }

    void Canvas::attach_default_viewport_2d(const std::string &id, Camera2D &camera) {
        if (pimpl->viewports.find(id) != pimpl->viewports.end()) {
            throw std::runtime_error("Viewport with provided ID already exists on the current camera");
        }

        Viewport2D viewport;
        viewport.top = 0;
        viewport.left = 0;
        viewport.bottom = 1;
        viewport.right = 1;
        viewport.scaling = Vector2f(1, 1);
        viewport.auto_aspect = true;
    }

    void Canvas::detach_viewport_2d(const std::string &id) {
        auto it = pimpl->viewports.find(id);
        if (it == pimpl->viewports.end()) {
            throw std::runtime_error("Viewport with provided ID does not exist on the current camera");
        }
        pimpl->viewports.erase(it);
    }
}
