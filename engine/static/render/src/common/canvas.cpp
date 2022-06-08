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

    static void _register_scene(pimpl_Canvas &pimpl, Scene &scene) {
        pimpl.scenes.push_back(&scene);

        std::sort(pimpl.scenes.begin(), pimpl.scenes.end(),
                  [](const auto *a, const auto *b) { return a->get_pimpl()->index < b->get_pimpl()->index; });
    }

    Canvas::Canvas(Window &window):
            pimpl(new pimpl_Canvas(window)) {
    }

    Canvas::~Canvas() {
        for (auto *scene : pimpl->scenes) {
            delete scene;
        }

        delete pimpl;
    }

    Window &Canvas::get_window(void) const {
        return pimpl->window;
    }

    const std::vector<Scene*> &Canvas::get_scenes(void) const {
        return pimpl->scenes;
    }

    Scene2D &Canvas::create_scene_2d(const int index) {
        auto *scene = new Scene2D(*this, Transform2D{}, index);

        _register_scene(*pimpl, *scene);

        return *scene;
    }

    void Canvas::remove_scene(Scene &scene) {
        if (&scene.get_canvas() != this) {
            throw std::invalid_argument("Supplied Scene does not belong to the Canvas");
        }

        remove_from_vector(pimpl->scenes, &scene);
    }
}
