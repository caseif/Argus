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
            tex_resource(nullptr) {
        parent.add_renderable(*this);
    }

    Renderable::~Renderable(void) {
        release_texture();
    }

    void Renderable::remove(void) {
        parent.remove_renderable(*this);

        delete this;
    }

    Transform const &Renderable::get_transform(void) const {
        return transform;
    }

    static void _fill_buffer(float *const buffer, Vertex const &vertex, const size_t offset) {
        buffer[offset + 0] = vertex.position.x;
        buffer[offset + 1] = vertex.position.y;
        buffer[offset + 2] = vertex.color.r;
        buffer[offset + 3] = vertex.color.g;
        buffer[offset + 4] = vertex.color.b;
        buffer[offset + 5] = vertex.color.a;
        buffer[offset + 6] = vertex.tex_coord.x;
        buffer[offset + 7] = vertex.tex_coord.y;
        buffer[offset + 8] = vertex.tex_coord.z;
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

        _fill_buffer(vertex_buffer, vertex, buffer_head);
        buffer_head += _VERTEX_LEN;
    }

    void Renderable::upload_buffer(size_t offset) {
        glBufferSubData(GL_ARRAY_BUFFER, offset, buffer_size * sizeof(float), vertex_buffer);
    }

    void Renderable::set_texture(std::string const &texture_uid) {
        release_texture();

        if (ResourceManager::get_global_resource_manager().get_resource(texture_uid, &tex_resource) != 0) {
            _ARGUS_WARN("Failed to set texture of Renderable: %s\n", get_error().c_str());
        }
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
