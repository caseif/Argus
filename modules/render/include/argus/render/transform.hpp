/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

namespace argus {
    /**
     * \brief Represents a vertex in 2D space containing a 2-dimensional spatial
     *        position, an RGBA color value, and 2-dimensional texture UV coordinates.
     */
    struct Vertex {
        /**
         * \brief The position of this vertex in 2D space.
         */
        Vector2f position;
        /**
         * \brief The normal of this vertex in 2D space.
         */
        Vector2f normal;
        /**
         * \brief The RGBA color of this vertex in [0,1] space.
         */
        Vector4f color;
        /**
         * \brief The texture coordinates of this vertex in UV-space.
         */
        Vector2f tex_coord;
    };

    class pimpl_Transform;

    /**
     * \brief A transformation in 2D space.
     *
     * \remark All member functions of this class are thread-safe.
     */
    class Transform {
        public:
            pimpl_Transform *pimpl;

            /**
             * \brief Constructs a Transform with no translation or rotation and
             *        1x scaling.
             */
            Transform(void);

            Transform(const Transform &rhs) noexcept;

            Transform(Transform &&rhs) noexcept;

            void operator=(const Transform &rhs) noexcept;

            ~Transform(void);

            /**
             * \brief Constructs a new 2D Transform with the given parameters.
             *
             * \param translation The translation in 2D space.
             * \param rotation The single-axis rotation.
             * \param scale The scale in 2D space.
             */
            Transform(const Vector2f &translation, const float rotation, const Vector2f &scale);

            /**
             * \brief Adds one Transform to another.
             *
             * The translation and rotation combinations are additive, while the
             * scale combination is multiplicative.
             *
             * \param rhs The Transform to add.
             *
             * \return The resulting Transform.
             */
            Transform operator +(const Transform rhs);

            /**
             * \brief Gets the translation component of this Transform.
             *
             * \return The translation component of this Transform.
             */
            Vector2f const get_translation(void);

            /**
             * \brief Sets the translation component of this Transform.
             *
             * \param translation The new translation for this Transform.
             */
            void set_translation(const Vector2f &translation);

            /**
             * \brief Adds the given value to this Transform's translation
             *        component.
             *
             * \param translation_delta The value to add to this Transform's
             *         translation component.
             */
            void add_translation(const Vector2f &translation_delta);

            /**
             * \brief Gets the rotation component of this Transform in radians.
             *
             * \return The rotation component of this Transform in radians.
             */
            const float get_rotation(void) const;

            /**
             * \brief Sets the rotation component of this Transform.
             *
             * \param rotation_radians The new rotation component for this Transform.
             */
            void set_rotation(const float rotation_radians);

            /**
             * \brief Adds the given value to this Transform's rotation
             *        component.
             *
             * \param rotation_radians The value in radians to add to this
             *         Transform's rotation component.
             */
            void add_rotation(const float rotation_radians);

            /**
             * \brief Gets the scale component of this Transform.
             *
             * \return The scale component of this Transform.
             */
            Vector2f const get_scale(void);

            /**
             * \brief Sets the scale component of this Transform.
             *
             * \param scale The new scale component for this Transform.
             */
            void set_scale(const Vector2f &scale);

            /**
             * \brief Returns an unmodifiable 4x4 matrix representation of this
             *        Transform.
             *
             * \return The matrix representation.
             */
            const mat4_flat_t &as_matrix(void);

            /**
             * \brief Copys a 4x4 matrix representation of this Transform into
             *        the given array.
             *
             * \param target The array to copy the matrix representation into.
             */
            void copy_matrix(mat4_flat_t target);

            /**
             * \brief Gets whether this transform has been modified since the
             *        last time the clear_dirty() function was invoked.
             *
             * \return Whether this transform is dirty.
             */
            const bool is_dirty(void) const;
    };
}
