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

// module renderer
#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/glext.hpp"

#include <set>
#include <SDL2/SDL_opengl.h>

namespace argus {

    using namespace glext;

    extern Shader g_group_transform_shader;

    static std::vector<const Shader*> _generate_initial_group_shaders(void) {
        std::vector<const Shader*> shaders;
        shaders.insert(shaders.cbegin(), &g_group_transform_shader);
        return shaders;
    }

    ShaderProgram RenderGroup::generate_initial_program(void) {
        std::vector<const Shader*> final_shaders;
        final_shaders.reserve(parent.shaders.size() + shaders.size());

        std::copy(parent.shaders.begin(), parent.shaders.end(), std::back_inserter(final_shaders));
        std::copy(shaders.begin(), shaders.end(), std::back_inserter(final_shaders));

        return ShaderProgram(std::move(final_shaders));
    }

    RenderGroup::RenderGroup(RenderLayer &parent):
            parent(parent),
            children(),
            transform(),
            renderable_factory(RenderableFactory(*this)),
            shaders(_generate_initial_group_shaders()),
            shader_program(generate_initial_program()),
            dirty_children(false),
            dirty_shaders(false),
            buffers_initialized(false),
            shaders_initialized(false),
            tex_handle(0) {
    }

    void RenderGroup::destroy(void) {
        if (&parent.root_group == this) {
            _ARGUS_FATAL("Cannot destroy root RenderGroup");
        }

        parent.remove_group(*this);

        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
    }

    Transform &RenderGroup::get_transform(void) {
        return transform;
    }

    void RenderGroup::rebuild_textures(bool force) {
        bool needs_rebuild = false;
        size_t max_width = 0;
        size_t max_height = 0;

        std::set<Resource*> seen_textures;
        for (Renderable *child : children) {
            if (child->tex_resource != nullptr) {
                seen_textures.insert(child->tex_resource);

                TextureData &tex_data = child->tex_resource->get_data<TextureData>();

                if (tex_data.width > max_width) {
                    max_width = tex_data.width;
                }
                if (tex_data.height > max_height) {
                    max_height = tex_data.height;
                }

                if (child->dirty_texture) {
                    needs_rebuild = true;
                }
            }
        }

        if (needs_rebuild) {
            if (glIsTexture(tex_handle)) {
                glDeleteTextures(1, &tex_handle);
            }

            glGenTextures(1, &tex_handle);
            glBindTexture(GL_TEXTURE_2D_ARRAY, tex_handle);

            if (!glIsTexture(tex_handle)) {
                _ARGUS_FATAL("Failed to gen texture while rebuilding texture array\n");
            }

            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, max_width, max_height, seen_textures.size(),
                    0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            texture_indices = {};
            unsigned int cur_index = 0;

            for (Resource *tex_res : seen_textures) {
                TextureData &tex_data = tex_res->get_data<TextureData>();

                if (!tex_data.is_prepared()) {
                    tex_data.prepare();
                }

                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, tex_data.buffer_handle);
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, cur_index, tex_data.width, tex_data.height, 1,
                        GL_RGBA, GL_UNSIGNED_BYTE, 0);
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

                texture_indices.insert({tex_res->uid, cur_index});

                cur_index++;
            }

            for (Renderable *child : children) {
                if (child->tex_resource != nullptr) {
                    auto it = texture_indices.find(child->tex_resource->uid);
                    _ARGUS_ASSERT(it != texture_indices.cend(), "Failed to get texture index after rebuilding\n");

                    TextureData &tex_data = child->tex_resource->get_data<TextureData>();
                    child->tex_index = it->second;
                    child->tex_max_uv = Vector2f{(float) tex_data.width / max_width, (float) tex_data.height / max_height};
                }

                child->dirty_texture = false;
            }
        }
    }

    void RenderGroup::update_buffer(void) {
        // if the children list is dirty, we'll just reinitialize the buffer entirely
        if (!buffers_initialized || dirty_children) {
            if (buffers_initialized) {
                glDeleteVertexArrays(1, &vao);
            }

            // init vertex array
            glGenVertexArrays(1, &vao);
        }

        glBindVertexArray(vao);

        if (!buffers_initialized || dirty_children) {
            if (buffers_initialized) {
                glDeleteBuffers(1, &vbo);
            }

            // init vertex buffer
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            GLenum err = glGetError();

            // compute how many vertices will be in this buffer
            vertex_count = 0;
            for (Renderable *const child : this->children) {
                vertex_count += child->get_vertex_count();
            }

            // allocate a new buffer
            glBufferData(GL_ARRAY_BUFFER, vertex_count * _VERTEX_LEN * _VERTEX_WORD_LEN, nullptr, GL_DYNAMIC_DRAW);

            // set up attribute metadata
            glEnableVertexAttribArray(_ATTRIB_LOC_POSITION);
            glEnableVertexAttribArray(_ATTRIB_LOC_COLOR);
            glEnableVertexAttribArray(_ATTRIB_LOC_TEXCOORD);

            GLintptr position_offset = 0;
            GLintptr color_offset = (_VERTEX_POSITION_LEN) * _VERTEX_WORD_LEN;
            GLintptr texcoord_offset = (_VERTEX_POSITION_LEN + _VERTEX_COLOR_LEN) * _VERTEX_WORD_LEN;
            GLint vertex_stride = _VERTEX_LEN * _VERTEX_WORD_LEN;

            glVertexAttribPointer(_ATTRIB_LOC_POSITION, _VERTEX_POSITION_LEN, GL_FLOAT, GL_FALSE,
                    vertex_stride, reinterpret_cast<GLvoid*>(position_offset));
            glVertexAttribPointer(_ATTRIB_LOC_COLOR, _VERTEX_COLOR_LEN, GL_FLOAT, GL_FALSE,
                    vertex_stride, reinterpret_cast<GLvoid*>(color_offset));
            glVertexAttribPointer(_ATTRIB_LOC_TEXCOORD, _VERTEX_TEXCOORD_LEN, GL_FLOAT, GL_FALSE,
                     vertex_stride, reinterpret_cast<GLvoid*>(texcoord_offset));
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
        }

        // push each child's data to the buffer, if applicable
        // we only push a child's data if the child list has changed, or the specific child's transform has changed
        size_t offset = 0;
        for (Renderable *const child : this->children) {
            if (child->transform.is_dirty() || dirty_children) {
                child->allocate_buffer(child->get_vertex_count() * _VERTEX_LEN);
                child->populate_buffer();
                glBufferSubData(GL_ARRAY_BUFFER, offset, child->buffer_size * sizeof(float), child->vertex_buffer);

                child->transform.clean();
            }
            offset += child->get_vertex_count();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        if (dirty_children) {
            dirty_children = false;
        }

        if (!buffers_initialized) {
            buffers_initialized = true;
        }
    }

    void RenderGroup::rebuild_shaders(void) {
        // check if any shader compilation is needed this frame
        if (!shaders_initialized || parent.dirty_shaders || dirty_shaders) {
            // check if there's an existing program that needs deletion
            if (shaders_initialized) {
                glDeleteProgram(shader_program.program_handle);
            }

            // create a superset of all shaders
            std::vector<const Shader*> shader_superlist;
            shader_superlist.insert(shader_superlist.end(), parent.shaders.cbegin(), parent.shaders.cend());
            shader_superlist.insert(shader_superlist.end(), shaders.cbegin(), shaders.cend());

            shader_program.update_shaders(shader_superlist);
            shader_program.link();
        } else if (shader_program.needs_rebuild) {
            shader_program.link();
        }

        if (!shaders_initialized || transform.is_dirty() || parent.transform.is_dirty()) {
            glUseProgram(shader_program.program_handle);
        }

        if (!shaders_initialized || transform.is_dirty()) {
            float transform_matrix[16];
            transform.to_matrix(transform_matrix);
            glUniformMatrix4fv(shader_program.get_uniform_location(_UNIFORM_GROUP_TRANSFORM), 1, GL_FALSE, transform_matrix);
            transform.clean();
        }

        if (!shaders_initialized || parent.transform.is_dirty()) {
            float transform_matrix[16];
            parent.transform.to_matrix(transform_matrix);
            glUniformMatrix4fv(shader_program.get_uniform_location(_UNIFORM_LAYER_TRANSFORM), 1, GL_FALSE, transform_matrix);
        }

        if (!shaders_initialized || transform.is_dirty() || parent.transform.is_dirty()) {
            glUseProgram(0);
        }

        if (dirty_shaders) {
            dirty_shaders = false;
        }

        if (!shaders_initialized) {
            shaders_initialized = true;
        }
    }

    void RenderGroup::draw(void) {
        rebuild_shaders();

        glUseProgram(shader_program.program_handle);

        rebuild_textures(false);

        update_buffer();

        glBindTexture(GL_TEXTURE_2D_ARRAY, tex_handle);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertex_count));
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        glUseProgram(0);
    }

    void RenderGroup::add_renderable(const Renderable &renderable) {
        //TODO: this is a hack
        children.insert(children.cbegin(), const_cast<Renderable*>(&renderable));
        dirty_children = true;
    }

    void RenderGroup::remove_renderable(const Renderable &renderable) {
        _ARGUS_ASSERT(&renderable.parent == this, "remove_renderable was passed Renderable with wrong parent");

        remove_from_vector(children, &renderable);
        dirty_children = true;
    }

    RenderableFactory &RenderGroup::get_renderable_factory(void) {
        return renderable_factory;
    }

}
