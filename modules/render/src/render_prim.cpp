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
#include "argus/render/render_prim.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/pimpl/render_prim.hpp"

#include <initializer_list>
#include <vector>

#include <cstdlib>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderPrim));

    RenderPrim::RenderPrim(const std::vector<Vertex> &vertices):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderPrim>(vertices)) {
    }

    RenderPrim::RenderPrim(const std::initializer_list<Vertex> vertices):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderPrim>(vertices)) {
    }

    RenderPrim::RenderPrim(const RenderPrim &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_RenderPrim>(rhs.pimpl->vertices)) {
    }

    RenderPrim::RenderPrim(RenderPrim &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    RenderPrim::~RenderPrim(void) {
        if (pimpl != nullptr) {
            g_pimpl_pool.free(pimpl);
        }
    }

    const size_t RenderPrim::get_vertex_count(void) const {
        return pimpl->vertices.size();
    }

}
