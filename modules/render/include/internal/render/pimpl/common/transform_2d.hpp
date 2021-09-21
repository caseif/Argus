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

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

// module render
#include "argus/render/common/transform.hpp"

#include <atomic>
#include <mutex>

namespace argus {
    struct pimpl_Transform2D {
        Vector2f translation;
        std::atomic<float> rotation;
        Vector2f scale;

        std::mutex translation_mutex;
        std::mutex scale_mutex;

        std::atomic_bool dirty;
        std::atomic_bool dirty_matrix;

        Matrix4 matrix_rep;

        pimpl_Transform2D(const Vector2f &translation, const float rotation, const Vector2f &scale):
            translation(translation),
            rotation(rotation),
            scale(scale),
            dirty(true),
            dirty_matrix(true) {
        }

        void set_dirty(void) {
            this->dirty = true;
            this->dirty_matrix = true;
        }
    };
}
