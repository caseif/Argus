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
#include "argus/renderer/render_group.hpp"
#include "argus/renderer/transform.hpp"

// module resman
#include "argus/resource_manager.hpp"

// module lowlevel
#include "argus/math.hpp"

#include <atomic>
#include <string>

#include <cstdio>

namespace argus {
    // forward declarations
    class RenderGroup;

    /**
     * \brief Represents an item to be rendered.
     *
     * Each item may have its own rendering properties, as well as a list of
     * child items. Child items will inherit the Transform of their respective
     * parent, which is added to their own.
     */
    class Renderable {
        friend class RenderGroup;

        private:
            /**
             * \brief Releases the handle on the underlying Resource for this
             *        Renderable's Texture.
             */
            void release_texture(void);

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

        protected:
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

            /**
             * \brief Constructs a new Renderable object.
             *
             * \param parent The RenderGroup parent to the new Renderable.
             */
            Renderable(RenderGroup &parent);

            ~Renderable(void);

            /**
             * \brief Re-allocates the vertex buffer to fit the given number of
             *        vertices.
             *
             * \param vertex_count The number of vertices to allocate for.
             *
             * \remark If the vertex buffer is already large enough to fit the
             *         given vertex count, this function will effectively be a
             *         no-op.
             * \remark The referenced vertex buffer exists in system memory, and
             *         is copied to graphics memory on invocation of
             *         RenderGroup#update_buffer(void).
             *
             * \sa Renderable#buffer_vertex(const Vertex&)
             * \sa Renderable#populate_buffer(void)
             */
            void allocate_buffer(const size_t vertex_count);

            /**
             * \brief Copies a Vertex to the vertex buffer.
             *
             * \param vertex The Vertex to buffer.
             *
             * \sa Renderable#allocate_buffer(const size_t)
             * \sa Renderable#populate_buffer(void)
             */
            void buffer_vertex(const Vertex &vertex);

            /**
             * \brief Populates the vertex buffer with this Renderable's current
             *        vertex data.
             *
             * \sa Renderable#allocate_buffer(const size_t)
             * \sa Renderable#buffer_vertex(const Vertex&)
             */
            virtual void populate_buffer(void) = 0;

            /**
             * \brief Gets the current vertex count of this Renderable.
             *
             * \return The current vertex count of this Renderable.
             */
            virtual const unsigned int get_vertex_count(void) const = 0;

            /**
             * \brief Removes this Renderable from memory.
             */
            virtual void free(void) = 0;

        public:
            /**
             * \brief Removes this Renderable from its parent RenderGroup and
             *        destroys it.
             *
             * \sa RenderGroup#remove_renderable(Renderable&)
             */
            void destroy(void);

            /**
             * \brief Gets the Transform of this Renderable.
             *
             * \return Transform This Renderable's Transform.
             */
            const Transform &get_transform(void) const;

            /**
             * \brief Applies the Texture with the given resource UID to this
             *        Renderable.
             *
             * This method will automatically attempt to load the Resource if
             * necessary.
             *
             * \param texture_uid The UID of the Resource containing the new
             *        texture.
             *
             * \throw ResourceException If the underlying texture Resource
             *        cannot be loaded.
             */
            void set_texture(const std::string &texture_uid);
    };
}
