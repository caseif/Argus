/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/common/render_layer.hpp"
#include "argus/render/common/transform.hpp"

#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Material;
    class Renderer;

    class RenderGroup2D;
    class RenderObject2D;
    class RenderPrim2D;

    struct pimpl_RenderLayer2D;

    /**
     * \brief Represents a layer to which geometry may be rendered.
     *
     * RenderLayers will be composited to the screen as multiple ordered layers
     * when a frame is rendered.
     */
    class RenderLayer2D : public RenderLayer {
        public:
            pimpl_RenderLayer2D *pimpl;

            /**
             * \brief Constructs a new RenderLayer.
             *
             * \param parent The Renderer parent to the layer.
             * \param transform The Transform of the layer.
             * \param index The index of the layer. Higher-indexed layers are
             *        rendered on top of lower-indexed ones.
             */
            RenderLayer2D(const Renderer &parent, const Transform2D &transform, int index);

            RenderLayer2D(const Renderer &parent, Transform2D &&transform, int index):
                RenderLayer2D(parent, transform, index) {
            }

            RenderLayer2D(const RenderLayer2D&) noexcept;

            RenderLayer2D(RenderLayer2D&&) noexcept;

            ~RenderLayer2D(void);

            pimpl_RenderLayer *get_pimpl(void) const override;

            /**
             * \brief Creates a new RenderGroup2D as a direct child of this
             *        layer.
             *
             * \param transform The relative transform of the new group.
             */
            RenderGroup2D &create_child_group(const Transform2D &transform);

            /**
             * \brief Creates a new RenderObject2D as a direct child of this
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
            RenderObject2D &create_child_object(const std::string &material, const std::vector<RenderPrim2D> &primitives,
                const Transform2D &transform);

            /**
             * \brief Removes the supplied RenderGroup2D from this layer,
             *        destroying it in the process.
             * \param group The group to remove and destroy.
             * \throw std::invalid_argument If the supplied RenderGroup is not a
             *        child of this layer.
             */
            void remove_child_group(RenderGroup2D &group);

            /**
             * \brief Removes the specified RenderObject2D from this layer,
             *        destroying it in the process.
             * \param object The RenderObject2D to remove and destroy.
             * \throw std::invalid_argument If the supplied RenderObject is not
             *        a child of this layer.
             */
            void remove_child_object(RenderObject2D &object);
    };
}
