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

#include "argus/lowlevel/memory.hpp"

#include "argus/render/2d/attached_viewport_2d.hpp"
#include "argus/render/common/attached_viewport.hpp"
#include "internal/render/common/attached_viewport.hpp"
#include "internal/render/pimpl/2d/attached_viewport_2d.hpp"

namespace argus {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_AttachedViewport2D));

    AttachedViewport2D::AttachedViewport2D(const Viewport &viewport, Camera2D &camera, uint32_t z_index):
        AttachedViewport(SceneType::TwoD),
        m_pimpl(&g_pimpl_pool.construct<pimpl_AttachedViewport2D>(g_viewport_next_id++, viewport, camera, z_index)) {
    }

    AttachedViewport2D::AttachedViewport2D(AttachedViewport2D &&rhs) noexcept:
        AttachedViewport(rhs.m_type),
        m_pimpl(rhs.m_pimpl) {
        rhs.m_pimpl = nullptr;
    }

    AttachedViewport2D::~AttachedViewport2D(void) {
        if (m_pimpl != nullptr) {
            g_pimpl_pool.destroy(m_pimpl);
        }
    }

    pimpl_AttachedViewport *AttachedViewport2D::get_pimpl(void) const {
        return m_pimpl;
    }

    Camera2D &AttachedViewport2D::get_camera(void) const {
        return m_pimpl->camera;
    }
}
