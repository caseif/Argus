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
#include "argus/render/transform.hpp"

#include <map>
#include <vector>

namespace argus {
    // forward declarations
    class Material;
    class RenderGroup;
    class RenderLayer;
    class RenderObject;
    class RenderPrim;
    class Renderer;

    struct pimpl_RenderGroup;

    /**
     * \brief Represents a set of RenderGroups and RenderObjects to be rendered
     *        together.
     *
     * A RenderGroup supplies a Transform which will be applied when rendering
     * child groups/objects in addition to their own local Transform.
     */
    class RenderGroup {
        public:
            pimpl_RenderGroup *pimpl;

            /**
             * \brief Constructs a new RenderGroup.
             *
             * \param parent_layer The RenderLayer this group belongs to.
             * \param parent_group The parent group this group belongs to, if
             *        applicable. This may be nullptr.
             */
            RenderGroup(const RenderLayer &parent_layer, RenderGroup *const parent_group, Transform &transform);

            RenderGroup(const RenderLayer &parent_layer, RenderGroup *const parent_group, Transform &&transform);

            RenderGroup(const RenderLayer &parent_layer, RenderGroup *const parent_group);

            RenderGroup(const RenderGroup&) noexcept;

            RenderGroup(RenderGroup&&) noexcept;

            ~RenderGroup(void);

            /**
             * \brief Gets the parent RenderLayer.
             *
             * \return The parent RenderLayer to this group.
             */
            const RenderLayer &get_parent_layer(void) const;

            /**
             * \brief Gets the parent RenderGroup, if applicable.
             *
             * \return The parent group to this one, or nullptr if this is a
             *         root group.
             */
            RenderGroup *const get_parent_group(void) const;

            /**
             * \brief Creates a new RenderGroup as a child of this group.
             *
             * \param transform The relative transform of the new group.
             */
            RenderGroup &create_child_group(Transform &transform);

            /**
             * \brief Creates a new RenderObject as a child of this group.
             *
             * \param material The Material to be used by the new object.
             * \param primitives The \link RenderPrim primitives \endlink
             *        comprising the new object.
             * \param transform The relative transform of the new object.
             */
            RenderObject &create_child_object(const Material &material, const std::vector<RenderPrim> &primitives,
                Transform &transform);

            /**
             * \brief Removes the supplied RenderGroup from this group,
             *        destroying it in the process.
             * \param group The group to remove and destroy.
             * \throw std::invalid_argument If the supplied RenderGroup is not a
             *        child of this group.
             */
            void remove_child_group(RenderGroup &group);

            /**
             * \brief Removes the specified RenderObject from this group,
             *        destroying it in the process.
             * \param object The RenderObject to remove and destroy.
             * \throw std::invalid_argument If the supplied RenderObject is not
             *        a child of this group.
             */
            void remove_child_object(RenderObject &object);

            /**
             * \brief Gets the local Transform of this group.
             *
             * \return The local Transform.
             *
             * \remark The returned Transform is local and, if this group is a
             *         child of another, does not necessarily reflect the
             *         group's absolute transform with respect to the
             *         RenderLayer containing the group.
             */
            Transform &get_transform(void) const;

            /**
             * Sets the local Transform of this group.
             *
             * \param transform The new local Transform for this group.
             */
            void set_transform(Transform &transform);
    };
}
