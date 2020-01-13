/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module renderer
#include "argus/renderer/render_layer.hpp"
#include "argus/renderer/renderable_factory.hpp"
#include "argus/renderer/renderable.hpp"
#include "argus/renderer/shader.hpp"
#include "argus/renderer/shader_program.hpp"
#include "argus/renderer/util/types.hpp"

#include <map>
#include <vector>

namespace argus {
    // forward declarations
    class RenderLayer;
    class RenderableFactory;
    class Renderable;
    class Shader;
    class ShaderProgram;

    struct pimpl_RenderGroup;

    /**
     * \brief Represents a group of \link Renderable Renderables \endlink to be
     *        rendered at once.
     *
     * A RenderGroup may contain both its own Transform and \link Shader Shaders
     * \endlink, which will be applied in conjunction with the respective
     * properties of its parent RenderLayer.
     */
    class RenderGroup {
        friend Renderable;
        friend RenderLayer;
        friend RenderableFactory;

        private:
            pimpl_RenderGroup *pimpl;

            /**
             * \brief Constructs a new RenderGroup.
             *
             * \param parent The RenderLayer which will serve as parent to the
             *        new group.
             */
            RenderGroup(RenderLayer &parent);

            RenderGroup(const RenderGroup &ref);

            /**
             * \brief Rebuilds the texture array associated with this group.
             *
             * \param force Whether the textures should be rebuilt regardless of
             *        whether they have been modified.
             */
            void rebuild_textures(bool force);

            /**
             * \brief Updates the vertex buffer and array objects associated
             *        with this group, flushing any changes to the child
             *        Renderable objects.
             */
            void update_buffer(void);

            /**
             * \brief Rebuilds this group's own and inherited
             *        \link Shader Shaders \endlink if needed, updating uniforms
             *        as rqeuired.
             */
            void rebuild_shaders(void);

            /**
             * \brief Draws this group to the screen.
             */
            void draw(void);

            /**
             * \brief Adds a Renderable as a child of this group.
             *
             * \param renderable The Renderable to add as a child.
             */
            void add_renderable(const Renderable &renderable);

            /**
             * \brief Removes a Renderable from this group's children list.
             *
             * \param renderable The Renderable to remove from the children
             *        list.
             *
             * \attention This does not de-allocate the Renderable object, which
             *            must be done separately.
             *
             * \sa Renderable#destroy(void)
             */
            void remove_renderable(const Renderable &renderable);

        public:
            /**
             * \brief Destroys this object.
             *
             * \warning This method destroys the object. No other methods
             *          should be invoked upon it afterward.
             */
            void destroy(void);

            /**
             * \brief Gets the local Transform of this group.
             *
             * \return The local Transform.
             *
             * \remark This Transform is local to the parent RenderLayer, and
             *         does not necessarily reflect the group's transform in
             *         absolute screen space.
             */
            Transform &get_transform(void);

            /**
             * \brief Returns a factory for creating
             *        \link Renderable Renderables \endlink attached to this
             *        RenderGroup.
             *
             * \return This RenderGroup's Renderable factory.
             */
            RenderableFactory &get_renderable_factory(void);

            /**
             * \brief Adds a local Shader to this group.
             *
             * \param shader The Shader to add.
             */
            void add_shader(const Shader &shader);

            /**
             * \brief Removes a local Shader from this group.
             *
             * \param shader The Shader to remove.
             */
            void remove_shader(const Shader &shader);
    };
}
