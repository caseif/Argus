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

#include "argus/render/common/transform.hpp"

namespace argus {
    // forward declarations
    struct pimpl_Light2D;

    enum class Light2DType {
        Point = 0,
    };

    class Light2D {
      public:
        pimpl_Light2D *m_pimpl;

        Light2D(Light2DType type, bool is_occludable, const Vector3f &color, float intensity,
                float attenuation_constant, const Transform2D &transform);

        Light2D(Handle handle, Light2DType type, bool is_occludable, const Vector3f &color, float intensity,
                float attenuation_constant, const Transform2D &transform);

        [[nodiscard]] Light2DType get_type(void) const;

        [[nodiscard]] bool is_occludable(void) const;

        [[nodiscard]] const Vector3f &get_color(void) const;

        void set_color(const Vector3f &color);

        [[nodiscard]] float get_intensity(void) const;

        void set_intensity(float intensity);

        [[nodiscard]] float get_attenuation_constant(void) const;

        void set_attenuation_constant(float attenuation_constant);

        [[nodiscard]] const Transform2D &get_transform(void) const;

        void set_transform(const Transform2D &transform);
    };
}
