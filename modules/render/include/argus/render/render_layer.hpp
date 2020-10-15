/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/transform.hpp"

#include <vector>

namespace argus {
    // forward declarations
    class Material;
    class RenderGroup;
    class RenderObject;
    class RenderPrim;
    class Renderer;

    struct pimpl_RenderLayer;

    /**
     * \brief Represents a layer to which geometry may be rendered.
     *
     * RenderLayers will be composited to the screen as multiple ordered layers
     * when a frame is rendered.
     */
    class RenderLayer {
        public:
            pimpl_RenderLayer *const pimpl;

            /**
             * \brief Constructs a new RenderLayer.
             *
             * \param parent The Renderer parent to the layer.
             * \param transform The Transform of the layer.
             * \param index The index of the layer. Higher-indexed layers are
             *        rendered on top of lower-indexed ones.
             */
            RenderLayer(const Renderer &parent, Transform &transform, const int index);

            RenderLayer(const Renderer &parent, Transform &&transform, const int index):
                RenderLayer(parent, transform, index) {
            }


            ~RenderLayer(void);

            /**
             * \brief Gets the parent Renderer of this layer.
             */
            const Renderer &get_parent_renderer(void) const;

            /**
             * \brief Creates a new RenderGroup as a direct child of this layer.
             *
             * \param transform The relative transform of the new group.
             */
            RenderGroup &create_child_group(Transform &transform);

            /**
             * \brief Creates a new RenderObject as a direct child of this
             *        layer.
             *
             * \param material The Material to be used by the new object.
             * \param primitives The primitives comprising the new object.
             * \param transform The relative transform of the new object.
             *
             * \remark Internally, the object will be created as a child of the
             *         implicit root RenderGroup contained by this layer. Thus,
             *         no RenderObject is truly without a parent group.
             */
            RenderObject &create_child_object(const Material &material, const std::vector<RenderPrim> &primitives,
                Transform &transform);

            /**
             * \brief Removes the supplied RenderGroup from this layer,
             *        destroying it in the process.
             * \param group The group to remove and destroy.
             * \throw std::invalid_argument If the supplied RenderGroup is not a
             *        child of this layer.
             */
            void remove_child_group(RenderGroup &group);

            /**
             * \brief Removes the specified RenderObject from this layer,
             *        destroying it in the process.
             * \param object The RenderObject to remove and destroy.
             * \throw std::invalid_argument If the supplied RenderObject is not
             *        a child of this layer.
             */
            void remove_child_object(RenderObject &object);

            /**
             * \brief Gets the Transform of this layer.
             *
             * \return The layer's Transform.
             */
            const Transform &get_transform(void) const;

            /**
             * \brief Sets the Transform of this layer.
             *
             * \param transform The new Transform.
             */
            void set_transform(Transform &transform);
    };
}
