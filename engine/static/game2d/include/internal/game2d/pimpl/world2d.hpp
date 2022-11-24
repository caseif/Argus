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

#pragma once

#include <map>
#include <string>

namespace argus {
    // forward declarations
    class Canvas;
    class Scene2D;

    class AnimatedSprite;
    class Sprite;

    struct pimpl_World2D {
        std::string id;
        Canvas &canvas;

        Scene2D *scene;
        Camera2D *camera;

        std::map<std::string, Sprite*> sprites;
        std::map<std::string, AnimatedSprite*> anim_sprites;

        pimpl_World2D(const std::string &id, Canvas &canvas):
            id(id),
            canvas(canvas) {
        }
    };
}
