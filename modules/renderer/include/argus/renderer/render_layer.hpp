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
#include "argus/renderer/render_group.hpp"
#include "argus/renderer/renderable_factory.hpp"
#include "argus/renderer/renderer.hpp"
#include "argus/renderer/shader.hpp"
#include "argus/renderer/transform.hpp"

#include <vector>

namespace argus {
    // forward declarations
    class RenderGroup;
    class RenderableFactory;
    class Renderer;
    class Shader;

    /**
     * \brief Represents a layer to which geometry may be rendered.
     *
     * RenderLayers will be composited to the screen as multiple ordered layers
     * when a frame is rendered.
     */
    class RenderLayer {
        friend class Renderer;
        friend class RenderGroup;
        friend class pimpl_RenderGroup;
        friend class RenderableFactory;

        private:
            /**
             * \brief The Renderer parent to this layer.
             */
            Renderer &parent_renderer;
            /**
             * \brief The priority of this layer.
             *
             * Higher-priority layers will be rendered later, on top of
             * lower-priority ones.
             */
            const int priority;

            /**
             * \brief The \link RenderGroup RenderGroups \endlink contained by
             *        this layer.
             */
            std::vector<RenderGroup*> children;
            /**
             * \brief The \link Shader Shaders \endlink contained by this layer.
             */
            std::vector<const Shader*> shaders;

            /**
             * \brief The implicit default RenderGroup of this layer.
             *
             * \remark A pointer to this RenderGroup is present in the children
             *         vector.
             */
            RenderGroup &def_group; // this depends on shaders being initialized

            /**
             * \brief The Transform of this RenderLayer.
             *
             * \sa RenderGroup#get_transform
             */
            Transform transform;

            /**
             * \brief Whether the shader list has been modified since it was
             *        last built.
             */
            bool dirty_shaders;

            /**
             * \brief Constructs a new RenderLayer.
             *
             * \param parent The Renderer parent to the layer.
             * \param priority The priority of the layer.
             *
             * \sa RenderLayer#priority
             */
            RenderLayer(Renderer &parent, const int priority);

            ~RenderLayer(void) = default;

            /**
             * \brief Renders this layer to the screen.
             */
            void render(void);

            /**
             * \brief Removes the given RenderGroup from this layer.
             *
             * \param group The RenderGroup to remove.
             */
            void remove_group(RenderGroup &group);

        public:
            /**
             * \brief Destroys this RenderLayer and removes it from the parent
             *        Renderer.
             */
            void destroy(void);

            /**
             * \brief Gets the Transform of this layer.
             *
             * \return The layer's Transform.
             */
            Transform &get_transform(void);

            /**
             * \brief Returns a factory for creating
             *        \link Renderable Renderables \endlink attached to this
             *        this RenderLayer's root RenderGroup.
             *
             * \returns This RenderLayer's root Renderable factory.
             */
            RenderableFactory &get_renderable_factory(void);

            /**
             * \brief Creates a new RenderGroup as a child of this layer.
             *
             * \param priority The priority of the new RenderGroup.
             *
             * \return RenderGroup The created RenderGroup.
             */
            RenderGroup &create_render_group(const int priority);

            /**
             * \brief Gets the default RenderGroup of this layer.
             */
            RenderGroup &get_default_group(void);

            /**
             * \brief Adds the given Shader to this layer.
             *
             * \param shader The Shader to add.
             */
            void add_shader(const Shader &shader);

            /**
             * \brief Removes the given Shader from this layer.
             *
             * \param shader The Shader to remove.
             */
            void remove_shader(const Shader &shader);
    };
}
