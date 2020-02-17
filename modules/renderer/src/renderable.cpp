/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/math.hpp"
#include "argus/memory.hpp"
#include "internal/lowlevel/logging.hpp"

// module resman
#include "argus/resource_manager.hpp"

// module renderer
#include "argus/renderer/render_group.hpp"
#include "argus/renderer/renderable.hpp"
#include "argus/renderer/transform.hpp"
#include "internal/renderer/glext.hpp"
#include "internal/renderer/defines.hpp"
#include "internal/renderer/pimpl/renderable.hpp"

#include <atomic>
#include <string>

#include <cstdlib>

namespace argus {

    using namespace glext;

    static AllocPool g_pimpl_pool(sizeof(pimpl_Renderable));

    Renderable::Renderable(RenderGroup &group):
            pimpl(&g_pimpl_pool.construct<pimpl_Renderable>(group)) {
        pimpl->vertex_buffer = nullptr;
        pimpl->buffer_size = 0;
        pimpl->max_buffer_size = 0;
        pimpl->buffer_head = 0;
        pimpl->tex_index = 0;
        pimpl->tex_max_uv = {1.0, 1.0};
        pimpl->tex_resource = nullptr;
        pimpl->dirty_texture = false;
        pimpl->parent.add_renderable(*this);
    }

    void Renderable::destroy(void) {
        release_texture();

        if (pimpl->max_buffer_size > 0) {
            ::free(pimpl->vertex_buffer);
        }

        pimpl->parent.remove_renderable(*this);

        pimpl->transform.~Transform();

        g_pimpl_pool.free(pimpl);

        this->free();
    }

    const Transform &Renderable::get_transform(void) const {
        return pimpl->transform;
    }

    static void _fill_buffer(float *const buffer, const Vertex &vertex, unsigned int tex_index, const size_t offset) {
    }

    void Renderable::allocate_buffer(const size_t vertex_count) {
        size_t new_size = vertex_count * _VERTEX_LEN;

        if (pimpl->max_buffer_size == 0) {
            pimpl->vertex_buffer = static_cast<float*>(malloc(new_size));
            pimpl->max_buffer_size = new_size;
        }

        if (new_size > pimpl->max_buffer_size) {
            pimpl->vertex_buffer = static_cast<float*>(realloc(pimpl->vertex_buffer, new_size));
            pimpl->max_buffer_size = new_size;
        }

        pimpl->buffer_size = new_size;
        pimpl->buffer_head = 0;
    }

    void Renderable::buffer_vertex(const Vertex &vertex) {
        if (pimpl->buffer_head + _VERTEX_LEN > pimpl->buffer_size) {
            _ARGUS_FATAL("Buffer overflow while buffering vertex (%zu > %zu)",
                    pimpl->buffer_head + _VERTEX_LEN, pimpl->buffer_size);
        }

        pimpl->vertex_buffer[pimpl->buffer_head + 0] = vertex.position.x;
        pimpl->vertex_buffer[pimpl->buffer_head + 1] = vertex.position.y;
        pimpl->vertex_buffer[pimpl->buffer_head + 2] = vertex.color.r;
        pimpl->vertex_buffer[pimpl->buffer_head + 3] = vertex.color.g;
        pimpl->vertex_buffer[pimpl->buffer_head + 4] = vertex.color.b;
        pimpl->vertex_buffer[pimpl->buffer_head + 5] = vertex.color.a;
        pimpl->vertex_buffer[pimpl->buffer_head + 6] = vertex.tex_coord.x * pimpl->tex_max_uv.x;
        pimpl->vertex_buffer[pimpl->buffer_head + 7] = vertex.tex_coord.y * pimpl->tex_max_uv.y;
        pimpl->vertex_buffer[pimpl->buffer_head + 8] = pimpl->tex_index;

        pimpl->buffer_head += _VERTEX_LEN;
    }

    void Renderable::set_texture(const std::string &texture_uid) {
        release_texture();

        pimpl->tex_resource = &ResourceManager::get_global_resource_manager().get_resource(texture_uid);

        pimpl->dirty_texture = true;
    }

    void Renderable::release_texture(void) {
        if (pimpl->tex_resource == nullptr) {
            return;
        }

        try {
            ResourceManager::get_global_resource_manager().get_resource(pimpl->tex_resource->uid);
            pimpl->tex_resource->release();
        } catch (ResourceException &ex) {
            _ARGUS_WARN("Previous texture %s for Renderable was invalid\n",
                    ((std::string) pimpl->tex_resource->uid).c_str());
        }
    }

}
