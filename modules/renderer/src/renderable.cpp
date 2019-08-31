// module lowlevel
#include "internal/logging.hpp"

// module core
#include "internal/core_util.hpp"

// module resman
#include "argus/resource_manager.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"
#include "internal/renderer_defines.hpp"

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

    Renderable::~Renderable(void) {
        release_texture();

        if (max_buffer_size > 0) {
            free(vertex_buffer);
        }
    }

    void Renderable::remove(void) {
        parent.remove_renderable(*this);

        delete this;
    }

    Transform const &Renderable::get_transform(void) const {
        return transform;
    }

    static void _fill_buffer(float *const buffer, Vertex const &vertex, unsigned int tex_index, const size_t offset) {
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

    void Renderable::buffer_vertex(Vertex const &vertex) {
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

    void Renderable::set_texture(std::string const &texture_uid) {
        release_texture();

        if (ResourceManager::get_global_resource_manager().get_resource(texture_uid, &tex_resource) != 0) {
            _ARGUS_WARN("Failed to set texture of Renderable: %s\n", get_error().c_str());
        }

        dirty_texture = true;
    }

    void Renderable::release_texture(void) {
        if (tex_resource == nullptr) {
            return;
        }

        Resource *res;
        if (ResourceManager::get_global_resource_manager().get_resource(tex_resource->uid, &res) == 0) {
            tex_resource->release();
        } else {
            _ARGUS_WARN("Previous texture %s for Renderable was invalid\n", ((std::string) tex_resource->uid).c_str());
        }
    }

}
