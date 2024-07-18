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

#include "argus/render/cabi/common/transform.h"

#include "argus/render/common/transform.hpp"

extern "C" {

argus_matrix_4x4_t argus_transform_2d_as_matrix(const ArgusTransform2d *transform, float anchor_x, float anchor_y) {
    auto cpp_mat = argus::Transform2D(
            *reinterpret_cast<const argus::Vector2f *>(&transform->translation),
            transform->rotation,
            *reinterpret_cast<const argus::Vector2f *>(&transform->scale)
    ).as_matrix(argus::Vector2f { anchor_x, anchor_y });
    argus_matrix_4x4_t c_mat {};
    memcpy(c_mat.cells, cpp_mat.data, sizeof(c_mat.cells));
    return c_mat;
}

argus_matrix_4x4_t argus_transform_2d_get_translation_matrix(const ArgusTransform2d *transform) {
    auto cpp_mat = argus::Transform2D(
            *reinterpret_cast<const argus::Vector2f *>(&transform->translation),
            transform->rotation,
            *reinterpret_cast<const argus::Vector2f *>(&transform->scale)
    ).get_translation_matrix();
    argus_matrix_4x4_t c_mat {};
    memcpy(c_mat.cells, cpp_mat.data, sizeof(c_mat.cells));
    return c_mat;
}

argus_matrix_4x4_t argus_transform_2d_get_rotation_matrix(const ArgusTransform2d *transform) {
    auto cpp_mat = argus::Transform2D(
            *reinterpret_cast<const argus::Vector2f *>(&transform->translation),
            transform->rotation,
            *reinterpret_cast<const argus::Vector2f *>(&transform->scale)
    ).get_rotation_matrix();
    argus_matrix_4x4_t c_mat {};
    memcpy(c_mat.cells, cpp_mat.data, sizeof(c_mat.cells));
    return c_mat;
}

argus_matrix_4x4_t argus_transform_2d_get_scale_matrix(const ArgusTransform2d *transform) {
    auto cpp_mat = argus::Transform2D(
            *reinterpret_cast<const argus::Vector2f *>(&transform->translation),
            transform->rotation,
            *reinterpret_cast<const argus::Vector2f *>(&transform->scale)
    ).get_scale_matrix();
    argus_matrix_4x4_t c_mat {};
    memcpy(c_mat.cells, cpp_mat.data, sizeof(c_mat.cells));
    return c_mat;
}

ArgusTransform2d argus_transform_2d_inverse(const ArgusTransform2d *transform) {
    return ArgusTransform2d {
        argus_vector_2f_t { -transform->translation.x, -transform->translation.y },
        transform->scale,
        -transform->rotation,
    };
}

}
