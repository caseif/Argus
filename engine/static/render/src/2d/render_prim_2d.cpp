/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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
#include <vector>

#include <cstdlib>

namespace argus {
    // forward declarations
    struct Vertex2D;

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderPrim2D));

    RenderPrim2D::RenderPrim2D(const std::vector<Vertex2D> &vertices):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderPrim2D>(vertices)) {
    }

    RenderPrim2D::RenderPrim2D(const std::initializer_list<Vertex2D> vertices):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderPrim2D>(vertices)) {
    }

    RenderPrim2D::RenderPrim2D(const RenderPrim2D &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_RenderPrim2D>(rhs.pimpl->vertices)) {
    }

    RenderPrim2D::RenderPrim2D(RenderPrim2D &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    RenderPrim2D::~RenderPrim2D(void) {
        if (pimpl != nullptr) {
            g_pimpl_pool.destroy(pimpl);
        }
    }

    size_t RenderPrim2D::get_vertex_count(void) const {
        return pimpl->vertices.size();
    }

}
