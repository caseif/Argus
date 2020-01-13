/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module renderer
#include "argus/renderer/transform.hpp"
#include "internal/renderer/types.hpp"
#include "internal/renderer/pimpl/render_layer.hpp"

#include <map>
#include <vector>

namespace argus {
    static std::vector<const Shader*> _merge_shaders(std::vector<const Shader*> &a, std::vector<const Shader*> &b) {
        std::vector<const Shader*> final_shaders;
        final_shaders.reserve(a.size() + b.size());

        std::copy(a.begin(), a.end(), std::back_inserter(final_shaders));
        std::copy(b.begin(), b.end(), std::back_inserter(final_shaders));
        return final_shaders;
    }

    struct pimpl_RenderGroup {
        /**
         * \brief The RenderLayer which this group belongs to.
         */
        RenderLayer &parent;
        /**
         * \brief The Renderable objects contained by this group.
         */
        std::vector<Renderable*> children;

        /**
         * \brief The Transform of this group.
         *
         * This will be combined with the Transform of the parent
         * RenderLayer.
         */
        Transform transform;

        /**
         * \brief The \link Shader Shaders \endlink to be applied to this
         *        group.
         *
         * These will be combined with the \link Shader Shaders \endlink of
         * the parent RenderLayer.
         */
        std::vector<const Shader*> shaders;

        /**
         * \brief A map of texture IDs to texture array indices.
         */
        std::map<std::string, unsigned int> texture_indices;

        /**
         * \brief The RenderableFactory associated with this group.
         *
         * RenderGroup and RenderableFactory objects always have a
         * one-to-one mapping.
         *
         * \sa RenderableFactory
         */
        RenderableFactory renderable_factory;

        /**
         * \brief The current total vertex count of this group.
         */
        size_t vertex_count;

        /**
         * \brief Whether the child Renderable list has been mutated since
         *        the list was last flushed to the underlying vertex buffer
         *        object.
         */
        bool dirty_children;
        /**
         * \brief Whether the shader list of either this object or its
         *        parent RenderLayer has been mutated since the full shader
         *        list was last compiled.
         */
        bool dirty_shaders;

        bool shaders_initialized;
        bool buffers_initialized;

        ShaderProgram shader_program;
        /**
         * A handle to the underlying vertex buffer object of this group.
         *
         * \attention The exact semantic meaning of this value is
         *            implementation-defined.
         */
        handle_t vbo;
        /**
         * A handle to the underlying vertex array object of this group.
         *
         * \attention The exact semantic meaning of this value is
         *            implementation-defined.
         */
        handle_t vao;
        /**
         * A handle to the underlying texture object of this group.
         *
         * \attention The exact semantic meaning of this value is
         *            implementation-defined.
         */
        handle_t tex_handle;

        pimpl_RenderGroup(RenderLayer &parent, RenderableFactory &&factory,
            std::vector<const Shader*> &&shaders):
                parent(parent),
                renderable_factory(factory),
                transform(),
                shaders(shaders),
                shader_program(_merge_shaders(parent.pimpl->shaders, shaders)) {
        }
    };
}
