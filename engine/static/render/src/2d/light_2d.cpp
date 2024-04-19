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

#include "argus/lowlevel/memory.hpp"

#include "argus/render/2d/light_2d.hpp"
#include "internal/render/pimpl/2d/light_2d.hpp"

namespace argus {
    static PoolAllocator g_alloc_pool(sizeof(pimpl_Light2D));

    Light2D::Light2D(Light2DType type, const Vector3f &color, float intensity, float decay_factor,
            const Transform2D &transform) :
        m_pimpl(&g_alloc_pool.construct<pimpl_Light2D>(type, color, intensity, decay_factor, transform)) {
    }

    Light2DType Light2D::get_type(void) const {
        return m_pimpl->type;
    }

    const Vector3f &Light2D::get_color(void) const {
        return m_pimpl->color;
    }

    void Light2D::set_color(const Vector3f &color) {
        m_pimpl->color = color;
    }

    float Light2D::get_intensity(void) const {
        return m_pimpl->intensity;
    }

    void Light2D::set_intensity(float intensity) {
        affirm_precond(intensity >= 0.0, "Light intensity must be >= 0");

        m_pimpl->intensity = intensity;
    }

    float Light2D::get_decay_factor(void) const {
        return m_pimpl->decay_factor;
    }

    void Light2D::set_decay_factor(float decay_factor) {
        affirm_precond(decay_factor >= 0, "Light decay factor must be >= 0");

        m_pimpl->decay_factor = decay_factor;
    }

    const Transform2D &Light2D::get_transform(void) const {
        return m_pimpl->transform;
    }

    void Light2D::set_transform(const Transform2D &transform) {
        m_pimpl->transform = transform;
    }
}
