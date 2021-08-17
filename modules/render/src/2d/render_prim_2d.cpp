/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module render
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
