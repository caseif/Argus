/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#pragma once

#include "argus/game2d/world2d_layer.hpp"

#include <array>
#include <map>
#include <string>
#include <utility>

namespace argus {
    // forward declarations
    class Canvas;

    class Scene2D;

    class Sprite;

    struct pimpl_World2D {
        std::string id;
        Canvas &canvas;
        const float scale_factor;
        Dirtiable<float> al_level;
        Dirtiable<Vector3f> al_color;

        World2DLayer *fg_layer = nullptr;
        std::array<World2DLayer *, MAX_BACKGROUND_LAYERS> bg_layers {};
        size_t bg_layers_count = 0;

        Dirtiable<Transform2D> abstract_camera;

        pimpl_World2D(std::string id, Canvas &canvas, float scale_factor):
            id(std::move(id)),
            canvas(canvas),
            scale_factor(scale_factor) {
            al_level = 1.0;
            al_color = { 1.0, 1.0, 1.0 };
        }
    };
}
