/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/logging.hpp"

// module core
#include "internal/core_util.hpp"

// module resman
#include "argus/resource_manager.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/renderer/glext.hpp"
#include "internal/renderer/renderer_defines.hpp"

#include <SDL2/SDL_opengl.h>

namespace argus {

    using namespace glext;

    Renderable::Renderable(RenderGroup &group):
            parent(group),
            transform(Transform()),
            vertex_buffer(nullptr),
            buffer_size(0),
            max_buffer_size(0),
            buffer_head(0),
            tex_index(0),
            tex_max_uv({1.0, 1.0}),
            tex_resource(nullptr),
            dirty_texture(false) {
        parent.add_renderable(*this);
    }

    void Renderable::destroy(void) {
        release_texture();

        if (max_buffer_size > 0) {
            ::free(vertex_buffer);
        }

        parent.remove_renderable(*this);

        this->transform.~Transform();

        this->free();
    }

    const Transform &Renderable::get_transform(void) const {
        return transform;
    }

    static void _fill_buffer(float *const buffer, const Vertex &vertex, unsigned int tex_index, const size_t offset) {
    }

    void Renderable::allocate_buffer(const size_t vertex_count) {
        size_t new_size = vertex_count * _VERTEX_LEN;

        if (max_buffer_size == 0) {
            vertex_buffer = static_cast<float*>(malloc(new_size));
            max_buffer_size = new_size;
        }

        if (new_size > max_buffer_size) {
            vertex_buffer = static_cast<float*>(realloc(vertex_buffer, new_size));
            max_buffer_size = new_size;
        }

        buffer_size = new_size;
        buffer_head = 0;
    }

    void Renderable::buffer_vertex(const Vertex &vertex) {
        if (buffer_head + _VERTEX_LEN > buffer_size) {
            _ARGUS_FATAL("Buffer overflow while buffering vertex (%lu > %lu)", buffer_head + _VERTEX_LEN, buffer_size);
        }

        vertex_buffer[buffer_head + 0] = vertex.position.x;
        vertex_buffer[buffer_head + 1] = vertex.position.y;
        vertex_buffer[buffer_head + 2] = vertex.color.r;
        vertex_buffer[buffer_head + 3] = vertex.color.g;
        vertex_buffer[buffer_head + 4] = vertex.color.b;
        vertex_buffer[buffer_head + 5] = vertex.color.a;
        vertex_buffer[buffer_head + 6] = vertex.tex_coord.x * tex_max_uv.x;
        vertex_buffer[buffer_head + 7] = vertex.tex_coord.y * tex_max_uv.y;
        vertex_buffer[buffer_head + 8] = tex_index;

        buffer_head += _VERTEX_LEN;
    }

    void Renderable::set_texture(const std::string &texture_uid) {
        release_texture();

        tex_resource = &ResourceManager::get_global_resource_manager().get_resource(texture_uid);

        dirty_texture = true;
    }

    void Renderable::release_texture(void) {
        if (tex_resource == nullptr) {
            return;
        }

        try {
            ResourceManager::get_global_resource_manager().get_resource(tex_resource->uid);
            tex_resource->release();
        } catch (ResourceException &ex) {
            _ARGUS_WARN("Previous texture %s for Renderable was invalid\n", ((std::string) tex_resource->uid).c_str());
        }
    }

}
