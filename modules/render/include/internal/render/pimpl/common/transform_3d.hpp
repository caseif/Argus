/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

// module render
#include "argus/render/common/transform.hpp"

#include <atomic>
#include <mutex>

namespace argus {
    struct pimpl_Transform3D {
        Vector3f translation;
        Vector3f rotation;
        Vector3f scale;

        std::mutex translation_mutex;
        std::mutex rotation_mutex;
        std::mutex scale_mutex;

        std::atomic_bool dirty;
        std::atomic_bool dirty_matrix;

        Matrix4 matrix_rep{};

        pimpl_Transform3D(const Vector3f &translation, const Vector3f rotation, const Vector3f &scale):
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
