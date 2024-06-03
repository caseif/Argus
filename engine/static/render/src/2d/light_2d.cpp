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

#include "argus/lowlevel/handle.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/render/2d/light_2d.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/pimpl/2d/light_2d.hpp"

namespace argus {
    static PoolAllocator g_alloc_pool(sizeof(pimpl_Light2D));

    static Handle _make_handle(Light2D *light) {
        return g_render_handle_table.create_handle(light);
    }

    Light2D::Light2D(Light2DType type, bool is_occludable, const Vector3f &color, LightParameters params,
            const Transform2D &transform):
        m_pimpl(&g_alloc_pool.construct<pimpl_Light2D>(_make_handle(this), type, is_occludable, color,
                params, transform)) {
        m_pimpl->transform.set_version_ref(m_pimpl->version);
    }

    Light2D::Light2D(Handle handle, Light2DType type, bool is_occludable, const Vector3f &color,
            LightParameters params, const Transform2D &transform):
        m_pimpl(&g_alloc_pool.construct<pimpl_Light2D>(handle, type, is_occludable, color,
                params, transform)) {
        g_render_handle_table.update_handle(handle, *this);
        m_pimpl->transform.set_version_ref(m_pimpl->version);
    }

    Handle Light2D::get_handle(void) const {
        return m_pimpl->handle;
    }

    Light2DType Light2D::get_type(void) const {
        return m_pimpl->type;
    }

    bool Light2D::is_occludable(void) const {
        return m_pimpl->is_occludable;
    }

    const Vector3f &Light2D::get_color(void) const {
        return m_pimpl->color;
    }

    void Light2D::set_color(const Vector3f &color) {
        m_pimpl->color = color;
        m_pimpl->version++;
    }

    const LightParameters &Light2D::get_parameters(void) const {
        return m_pimpl->params;
    }

    void Light2D::set_parameters(LightParameters params) {
        m_pimpl->params = params;
        m_pimpl->version++;
    }

    const Transform2D &Light2D::get_transform(void) const {
        return m_pimpl->transform;
    }

    void Light2D::set_transform(const Transform2D &transform) {
        m_pimpl->transform = transform;
        m_pimpl->version++;
    }
}
