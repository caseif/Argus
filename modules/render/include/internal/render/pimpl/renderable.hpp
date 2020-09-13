/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module resman
#include "argus/resman.hpp"

// module render
#include "argus/render/render_group.hpp"
#include "argus/render/transform.hpp"

#include <atomic>

#include <cstddef>

namespace argus {
    struct pimpl_Renderable {
        /**
         * \brief The raw vertex buffer data for this Renderable.
         */
        float *vertex_buffer;
        /**
         * \brief The current offset into the vertex buffer.
         *
         * \remark This is used for writing data to the buffer.
         */
        size_t buffer_head;
        /**
         * \brief The current number of elements in the vertex buffer.
         */
        size_t buffer_size;
        /**
         * \brief The current capacity in elements of the vertex buffer.
         */
        size_t max_buffer_size;
        /**
         * \brief The index of this Renderable's texture in the parent
         *        RenderGroup's texture array.
         */
        unsigned int tex_index;
        /**
         * \brief The UV coordinates of this Renderable's texture's
         *        bottom-right corner with respect to the parent
         *        RenderGroup's underlying texture array.
         */
        Vector2f tex_max_uv;
        /**
         * \brief Whether the texture has been modified since being flushed
         *        to the parent RenderGroup.
         */
        std::atomic_bool dirty_texture;
        /**
         * \brief The parent RenderGroup of this Renderable.
         */
        RenderGroup &parent;

        /**
         * \brief This Renderable's current Transform.
         */
        Transform transform;

        /**
         * \brief The Resource containing the texture to be applied to this
         *        Renderable.
         *
         * \remark This may be `nullptr` if no Texture is to be applied.
         */
        Resource *tex_resource;

        pimpl_Renderable(RenderGroup &parent):
                parent(parent),
                transform({}) {
        }
    };
}
