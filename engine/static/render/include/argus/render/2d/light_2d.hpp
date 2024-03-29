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

    class Light2D {
      private:
        //TODO
      public:
        pimpl_Light2D *m_pimpl;

        Light2D(const Vector3f &color, float intensity, float decay_factor, const Transform2D &transform);

        [[nodiscard]] const Vector3f &get_color(void) const;

        void set_color(const Vector3f &color);

        [[nodiscard]] float get_intensity(void) const;

        void set_intensity(float intensity);

        [[nodiscard]] float get_decay_factor(void) const;

        void set_decay_factor(float decay_factor);

        [[nodiscard]] const Transform2D &get_transform(void) const;

        void set_transform(const Transform2D &transform);
    };
}
