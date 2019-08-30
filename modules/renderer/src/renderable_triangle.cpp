#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

#define _TRIANGLE_VERTICES 3

namespace argus {

    using glext::glBufferSubData;

    RenderableTriangle::RenderableTriangle(RenderGroup &parent, Vertex const &corner_1, Vertex const &corner_2, Vertex const &corner_3):
            Renderable(parent),
            corner_1(corner_1),
            corner_2(corner_2),
            corner_3(corner_3) {
    }

    static void _fill_buffer(float *buffer, Vertex const &vertex, const size_t offset) {
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

    void RenderableTriangle::render(handle_t buffer_handle, const size_t offset) const {
        float buffer_data[_TRIANGLE_VERTICES * _VERTEX_LEN];

        _fill_buffer(buffer_data, corner_1, 0);
        _fill_buffer(buffer_data, corner_2, _VERTEX_LEN * 1);
        _fill_buffer(buffer_data, corner_3, _VERTEX_LEN * 2);

        if (tex_resource != nullptr) {
            TextureData &tex_data = tex_resource->get_data<TextureData>();
            if (!tex_data.is_ready()) {
                tex_data.upload_to_gpu();
            }
        }

        glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(buffer_data), buffer_data);
    }

    const unsigned int RenderableTriangle::get_vertex_count(void) const {
        return _TRIANGLE_VERTICES;
    }

}
