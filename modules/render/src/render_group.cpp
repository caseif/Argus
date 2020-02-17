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

// module core
#include "internal/core/core_util.hpp"

// module resman
#include "argus/resource_manager.hpp"

// module render
#include "argus/render/render_group.hpp"
#include "argus/render/render_layer.hpp"
#include "argus/render/renderable.hpp"
#include "argus/render/renderable_factory.hpp"
#include "argus/render/shader.hpp"
#include "argus/render/shader_program.hpp"
#include "argus/render/texture_data.hpp"
#include "argus/render/transform.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/glext.hpp"
#include "internal/render/pimpl/render_group.hpp"
#include "internal/render/pimpl/render_layer.hpp"
#include "internal/render/pimpl/renderable.hpp"
#include "internal/render/pimpl/shader_program.hpp"
#include "internal/render/pimpl/texture_data.hpp"

#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include <atomic>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <cstddef>

namespace argus {

    using namespace glext;

    extern Shader g_group_transform_shader;

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderGroup));

    static std::vector<const Shader*> _generate_initial_group_shaders(void) {
        std::vector<const Shader*> shaders;
        shaders.insert(shaders.cbegin(), &g_group_transform_shader);
        return shaders;
    }

    RenderGroup::RenderGroup(const RenderGroup &group): RenderGroup(group.pimpl->parent) {
    }

    RenderGroup::RenderGroup(RenderLayer &parent):
            pimpl(new pimpl_RenderGroup(
                parent,
                std::move(RenderableFactory(*this)),
                std::move(_generate_initial_group_shaders())
            )) {
            pimpl->children = {};
            pimpl->shaders = _generate_initial_group_shaders();
            pimpl->dirty_children = false;
            pimpl->dirty_shaders = false;
            pimpl->buffers_initialized = false;
            pimpl->shaders_initialized = false;
            pimpl->tex_handle = 0;
    }

    void RenderGroup::destroy(void) {
        if (pimpl->parent.pimpl->def_group == this) {
            _ARGUS_FATAL("Cannot destroy root RenderGroup");
        }

        pimpl->parent.remove_group(*this);

        glDeleteVertexArrays(1, &pimpl->vao);
        glDeleteBuffers(1, &pimpl->vbo);

        g_pimpl_pool.free(pimpl);
    }

    Transform &RenderGroup::get_transform(void) {
        return pimpl->transform;
    }

    void RenderGroup::rebuild_textures(bool force) {
        bool needs_rebuild = false;
        size_t max_width = 0;
        size_t max_height = 0;

        std::set<Resource*> seen_textures;
        for (Renderable *child : pimpl->children) {
            if (child->pimpl->tex_resource != nullptr) {
                seen_textures.insert(child->pimpl->tex_resource);

                TextureData &tex_data = child->pimpl->tex_resource->get_data<TextureData>();

                if (tex_data.width > max_width) {
                    max_width = tex_data.width;
                }
                if (tex_data.height > max_height) {
                    max_height = tex_data.height;
                }

                if (child->pimpl->dirty_texture) {
                    needs_rebuild = true;
                }
            }
        }

        if (needs_rebuild) {
            if (glIsTexture(pimpl->tex_handle)) {
                glDeleteTextures(1, &pimpl->tex_handle);
            }

            glGenTextures(1, &pimpl->tex_handle);
            glBindTexture(GL_TEXTURE_2D_ARRAY, pimpl->tex_handle);

            if (!glIsTexture(pimpl->tex_handle)) {
                _ARGUS_FATAL("Failed to gen texture while rebuilding texture array\n");
            }

            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, max_width, max_height, seen_textures.size(),
                    0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            pimpl->texture_indices = {};
            unsigned int cur_index = 0;

            for (Resource *tex_res : seen_textures) {
                TextureData &tex_data = tex_res->get_data<TextureData>();

                if (!tex_data.is_prepared()) {
                    tex_data.prepare();
                }

                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, tex_data.pimpl->buffer_handle);
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, cur_index, tex_data.width, tex_data.height, 1,
                        GL_RGBA, GL_UNSIGNED_BYTE, 0);
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

                pimpl->texture_indices.insert({tex_res->uid, cur_index});

                cur_index++;
            }

            for (Renderable *child : pimpl->children) {
                if (child->pimpl->tex_resource != nullptr) {
                    auto it = pimpl->texture_indices.find(child->pimpl->tex_resource->uid);
                    _ARGUS_ASSERT(it != pimpl->texture_indices.cend(),
                            "Failed to get texture index after rebuilding\n");

                    TextureData &tex_data = child->pimpl->tex_resource->get_data<TextureData>();
                    child->pimpl->tex_index = it->second;
                    child->pimpl->tex_max_uv = Vector2f{
                            (float) tex_data.width / max_width,
                            (float) tex_data.height / max_height
                    };
                }

                child->pimpl->dirty_texture = false;
            }
        }
    }

    void RenderGroup::update_buffer(void) {
        // if the children list is dirty, we'll just reinitialize the buffer entirely
        if (!pimpl->buffers_initialized || pimpl->dirty_children) {
            if (pimpl->buffers_initialized) {
                glDeleteVertexArrays(1, &pimpl->vao);
            }

            // init vertex array
            glGenVertexArrays(1, &pimpl->vao);
        }

        glBindVertexArray(pimpl->vao);

        if (!pimpl->buffers_initialized || pimpl->dirty_children) {
            if (pimpl->buffers_initialized) {
                glDeleteBuffers(1, &pimpl->vbo);
            }

            // init vertex buffer
            glGenBuffers(1, &pimpl->vbo);
            glBindBuffer(GL_ARRAY_BUFFER, pimpl->vbo);
            GLenum err = glGetError();

            // compute how many vertices will be in this buffer
            pimpl->vertex_count = 0;
            for (Renderable *const child : pimpl->children) {
                pimpl->vertex_count += child->get_vertex_count();
            }

            // allocate a new buffer
            glBufferData(GL_ARRAY_BUFFER, pimpl->vertex_count * _VERTEX_LEN * _VERTEX_WORD_LEN, nullptr,
                    GL_DYNAMIC_DRAW);

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
            glBindBuffer(GL_ARRAY_BUFFER, pimpl->vbo);
        }

        // push each child's data to the buffer, if applicable
        // we only push a child's data if the child list has changed, or the specific child's transform has changed
        size_t offset = 0;
        for (Renderable *const child : pimpl->children) {
            if (child->pimpl->transform.is_dirty() || pimpl->dirty_children) {
                child->allocate_buffer(child->get_vertex_count() * _VERTEX_LEN);
                child->populate_buffer();
                glBufferSubData(GL_ARRAY_BUFFER, offset, child->pimpl->buffer_size * sizeof(float),
                        child->pimpl->vertex_buffer);

                child->pimpl->transform.clean();
            }
            offset += child->pimpl->buffer_size * sizeof(float);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        if (pimpl->dirty_children) {
            pimpl->dirty_children = false;
        }

        if (!pimpl->buffers_initialized) {
            pimpl->buffers_initialized = true;
        }
    }

    void RenderGroup::rebuild_shaders(void) {
        // check if any shader compilation is needed this frame
        if (!pimpl->shaders_initialized || pimpl->parent.pimpl->dirty_shaders || pimpl->dirty_shaders) {
            // check if there's an existing program that needs deletion
            if (pimpl->shaders_initialized) {
                pimpl->shader_program.delete_program();
            }

            // create a superset of all shaders
            std::vector<const Shader*> shader_superlist;
            shader_superlist.insert(shader_superlist.end(),
                    pimpl->parent.pimpl->shaders.cbegin(), pimpl->parent.pimpl->shaders.cend());
            shader_superlist.insert(shader_superlist.end(), pimpl->shaders.cbegin(), pimpl->shaders.cend());

            pimpl->shader_program.update_shaders(shader_superlist);
            pimpl->shader_program.link();
        } else if (pimpl->shader_program.pimpl->needs_rebuild) {
            pimpl->shader_program.link();
        }

        if (!pimpl->shaders_initialized || pimpl->transform.is_dirty() || pimpl->parent.pimpl->transform.is_dirty()) {
            glUseProgram(pimpl->shader_program.pimpl->program_handle);
        }

        if (!pimpl->shaders_initialized || pimpl->transform.is_dirty()) {
            float transform_matrix[16];
            pimpl->transform.to_matrix(transform_matrix);
            glUniformMatrix4fv(pimpl->shader_program.get_uniform_location(_UNIFORM_GROUP_TRANSFORM), 1, GL_FALSE,
                    transform_matrix);
            pimpl->transform.clean();
        }

        if (!pimpl->shaders_initialized || pimpl->parent.pimpl->transform.is_dirty()) {
            float transform_matrix[16];
            pimpl->parent.pimpl->transform.to_matrix(transform_matrix);
            glUniformMatrix4fv(pimpl->shader_program.get_uniform_location(_UNIFORM_LAYER_TRANSFORM), 1, GL_FALSE,
                    transform_matrix);
        }

        if (!pimpl->shaders_initialized || pimpl->transform.is_dirty() || pimpl->parent.pimpl->transform.is_dirty()) {
            glUseProgram(0);
        }

        if (pimpl->dirty_shaders) {
            pimpl->dirty_shaders = false;
        }

        if (!pimpl->shaders_initialized) {
            pimpl->shaders_initialized = true;
        }
    }

    void RenderGroup::draw(void) {
        rebuild_shaders();

        glUseProgram(pimpl->shader_program.pimpl->program_handle);

        rebuild_textures(false);

        update_buffer();

        glBindTexture(GL_TEXTURE_2D_ARRAY, pimpl->tex_handle);
        glBindVertexArray(pimpl->vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(pimpl->vertex_count));
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        glUseProgram(0);
    }

    void RenderGroup::add_renderable(const Renderable &renderable) {
        //TODO: this is a hack
        pimpl->children.insert(pimpl->children.cbegin(), const_cast<Renderable*>(&renderable));
        pimpl->dirty_children = true;
    }

    void RenderGroup::remove_renderable(const Renderable &renderable) {
        _ARGUS_ASSERT(&renderable.pimpl->parent == this, "remove_renderable was passed Renderable with wrong parent");

        remove_from_vector(pimpl->children, &renderable);
        pimpl->dirty_children = true;


    }

    RenderableFactory &RenderGroup::get_renderable_factory(void) {
        return pimpl->renderable_factory;
    }

}
