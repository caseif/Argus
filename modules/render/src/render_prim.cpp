/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/render_prim.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/glext.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/pimpl/render_prim.hpp"

#include <vector>

#include <cstdlib>

namespace argus {

    using namespace glext;

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderPrim));

    RenderPrim::RenderPrim(const std::vector<Vertex> &vertices):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderPrim>(vertices)) {
        pimpl->vertex_buffer = nullptr;

        pimpl->vertex_buffer = static_cast<float*>(malloc(vertices.size() * _VERTEX_LEN));

        for (size_t i = 0; i < vertices.size(); i++) {
            const Vertex &vertex = vertices.at(i);
            pimpl->vertex_buffer[i * _VERTEX_LEN + 0] = vertex.position.x;
            pimpl->vertex_buffer[i * _VERTEX_LEN + 1] = vertex.position.y;
            pimpl->vertex_buffer[i * _VERTEX_LEN + 2] = vertex.color.r;
            pimpl->vertex_buffer[i * _VERTEX_LEN + 3] = vertex.color.g;
            pimpl->vertex_buffer[i * _VERTEX_LEN + 4] = vertex.color.b;
            pimpl->vertex_buffer[i * _VERTEX_LEN + 5] = vertex.color.a;
            pimpl->vertex_buffer[i * _VERTEX_LEN + 6] = vertex.tex_coord.x;
            pimpl->vertex_buffer[i * _VERTEX_LEN + 7] = vertex.tex_coord.y;
        }
    }

    RenderPrim::~RenderPrim(void) {
        g_pimpl_pool.free(pimpl);
    }

    const size_t RenderPrim::get_vertex_count(void) const {
        return pimpl->vertices.size();
    }

    const float *RenderPrim::get_buffered_data(void) const {
        return pimpl->vertex_buffer;
    }

}
