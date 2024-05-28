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

#pragma once

namespace argus {
    struct pimpl_Light2D {
        Handle handle;
        Light2DType type;
        bool is_occludable;
        Vector3f color;
        LightParameters params;
        Transform2D transform;

        uint16_t version;

        pimpl_Light2D(Handle handle, Light2DType type, bool is_occludable, Vector3f color,
                LightParameters params, const Transform2D &transform) :
            handle(handle),
            type(type),
            is_occludable(is_occludable),
            color(color),
            params(params),
            transform(transform),
            version(1) {
        }
    };
}
