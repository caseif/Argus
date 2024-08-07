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

#include "argus/render/2d/render_prim_2d.hpp"
#include "internal/render/pimpl/2d/render_prim_2d.hpp"

#include <initializer_list>
#include <iterator>
#include <vector>

#include <cstdlib>

namespace argus {
    // forward declarations
    struct Vertex2D;

    static PoolAllocator g_pimpl_pool(sizeof(pimpl_RenderPrim2D));

    RenderPrim2D::RenderPrim2D(const std::vector<Vertex2D> &vertices) {
        m_pimpl = &g_pimpl_pool.construct<pimpl_RenderPrim2D>(vertices);
    }

    RenderPrim2D::RenderPrim2D(std::initializer_list<Vertex2D> vertices):
        RenderPrim2D(std::vector<Vertex2D>(vertices)) {
    }

    RenderPrim2D::RenderPrim2D(const RenderPrim2D &rhs) noexcept:
        m_pimpl(&g_pimpl_pool.construct<pimpl_RenderPrim2D>(rhs.m_pimpl->vertices)) {
    }

    RenderPrim2D::RenderPrim2D(RenderPrim2D &&rhs) noexcept:
        m_pimpl(rhs.m_pimpl) {
        rhs.m_pimpl = nullptr;
    }

    RenderPrim2D &RenderPrim2D::operator=(const RenderPrim2D &rhs) {
        return *new(this) RenderPrim2D(rhs);
    }

    RenderPrim2D &RenderPrim2D::operator=(RenderPrim2D &&rhs) {
        return *new(this) RenderPrim2D(rhs);
    }

    RenderPrim2D::~RenderPrim2D(void) {
        if (m_pimpl != nullptr) {
            g_pimpl_pool.destroy(m_pimpl);
        }
    }

    size_t RenderPrim2D::get_vertex_count(void) const {
        return m_pimpl->vertices.size();
    }

    const std::vector<Vertex2D> &RenderPrim2D::get_vertices(void) const {
        return m_pimpl->vertices;
    }

    size_t RenderPrim2D::get_frame_count(void) const {
        return m_pimpl->frame_count;
    }
}
