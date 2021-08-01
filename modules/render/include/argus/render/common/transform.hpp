/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

namespace argus {
    struct pimpl_Transform2D;
    struct pimpl_Transform3D;

    /**
     * \brief A transformation in 2D space.
     *
     * \remark All member functions of this class are thread-safe.
     */
    class Transform2D {
        public:
            pimpl_Transform2D *pimpl;

            /**
             * \brief Constructs a transform with no translation or rotation and
             *        1x scaling.
             */
            Transform2D(void);

            Transform2D(const Transform2D &rhs) noexcept;

            Transform2D(Transform2D &&rhs) noexcept;

            Transform2D &operator =(const Transform2D &rhs) noexcept;

            ~Transform2D(void);

            /**
             * \brief Constructs a new 2D transform with the given parameters.
             *
             * \param translation The translation in 2D space.
             * \param rotation The single-axis rotation.
             * \param scale The scale in 2D space.
             */
            Transform2D(const Vector2f &translation, float rotation, const Vector2f &scale);

            /**
             * \brief Adds one transform to another.
             *
             * The translation and rotation combinations are additive, while the
             * scale combination is multiplicative.
             *
             * \param rhs The transform to add.
             *
             * \return The resulting transform.
             */
            Transform2D operator +(const Transform2D &rhs) const;

            /**
             * \brief Gets the translation component of the transform.
             *
             * \return The translation component of the transform.
             */
            Vector2f get_translation(void) const;

            /**
             * \brief Sets the translation component of the transform.
             *
             * \param translation The new translation for the transform.
             */
            void set_translation(const Vector2f &translation);

            /**
             * Sets the translation component of the transform.
             *
             * \param x The new x-translation for the transform.
             * \param y The new y-translation for the transform.
             */
            void set_translation(float x, float y);

            /**
             * \brief Adds the given value to the transform's translation
             *        component.
             *
             * \param translation_delta The value to add to the transform's
             *         translation.
             */
            void add_translation(const Vector2f &translation_delta);

            /**
             * \brief Adds the given value to the transform's translation
             *        component.
             *
             * \param x_delta The value to add to the transform's translation on
             *        the x-axis;
             * \param y_delta The value to add to the transform's translation on
             *        the y-axis.
             */
            void add_translation(float x_delta, float y_delta);

            /**
             * \brief Gets the rotation component of the transform in radians.
             *
             * \return The rotation component of the transform in radians.
             */
            float get_rotation(void) const;

            /**
             * \brief Sets the rotation component of the transform.
             *
             * \param rotation_radians The new rotation component for the transform.
             */
            void set_rotation(float rotation_radians);

            /**
             * \brief Adds the given value to the transform's rotation
             *        component.
             *
             * \param rotation_radians The value in radians to add to this
             *         transform's rotation component.
             */
            void add_rotation(float rotation_radians);

            /**
             * \brief Gets the scale component of the transform.
             *
             * \return The scale component of the transform.
             */
            Vector2f get_scale(void) const;

            /**
             * \brief Sets the scale component of the transform.
             *
             * \param scale The new scale for the transform.
             */
            void set_scale(const Vector2f &scale);

            /**
             * \brief Sets the scale component of the transform.
             *
             * \param x The new x-scale for the transform.
             * \param y The new y-scale for the transform.
             */
            void set_scale(float x, float y);

            /**
             * \brief Returns an unmodifiable 4x4 matrix representation of this
             *        transform.
             *
             * \return The matrix representation.
             */
            const mat4_flat_t &as_matrix(void);

            /**
             * \brief Copys a 4x4 matrix representation of the transform into
             *        the given array.
             *
             * \param target The array to copy the matrix representation into.
             */
            void copy_matrix(mat4_flat_t &target);
    };

    /**
     * \brief A transformation in 3D space.
     *
     * \remark All member functions of this class are thread-safe.
     */
    class Transform3D {
        public:
            pimpl_Transform3D *pimpl;

            /**
             * \brief Constructs a transform with no translation or rotation and
             *        1x scaling.
             */
            Transform3D(void);

            Transform3D(const Transform3D &rhs) noexcept;

            Transform3D(Transform3D &&rhs) noexcept;

            Transform3D &operator =(const Transform3D &rhs) noexcept;

            ~Transform3D(void);

            /**
             * \brief Constructs a new 3D transform with the given parameters.
             *
             * \param translation The translation in 3D space.
             * \param rotation The rotation in 3D space in the order (pitch,
             *        yaw, roll).
             * \param scale The scale in 3D space.
             */
            Transform3D(const Vector3f &translation, const Vector3f &rotation, const Vector3f &scale);

            /**
             * \brief Adds one transform to another.
             *
             * The translation and rotation combinations are additive, while the
             * scale combination is multiplicative.
             *
             * \param rhs The transform to add.
             *
             * \return The resulting transform.
             */
            Transform3D operator +(const Transform3D &rhs) const;

            /**
             * \brief Gets the translation component of the transform.
             *
             * \return The translation component of the transform.
             */
            Vector3f get_translation(void) const;

            /**
             * \brief Sets the translation component of the transform.
             *
             * \param translation The new translation for the transform.
             */
            void set_translation(const Vector3f &translation);

            /**
             * Sets the translation component of the transform.
             *
             * \param x The new x-translation for the transform.
             * \param y The new y-translation for the transform.
             * \param y The new z-translation for the transform.
             */
            void set_translation(float x, float y, float z);

            /**
             * \brief Adds the given value to the transform's translation
             *        component.
             *
             * \param translation_delta The value to add to the transform's
             *         translation.
             */
            void add_translation(const Vector3f &translation_delta);

            /**
             * \brief Adds the given value to the transform's translation
             *        component.
             *
             * \param x_delta The value to add to the transform's translation on
             *        the x-axis;
             * \param y_delta The value to add to the transform's translation on
             *        the y-axis.
             * \param z_delta The value to add to the transform's translation on
             *        the y-axis.
             */
            void add_translation(float x_delta, float y_delta, float z_delta);

            /**
             * \brief Gets the rotation component of the transform in radians.
             *
             * \return The rotation component of the transform in radians in the
             *         order (pitch, yaw, roll).
             */
            Vector3f get_rotation(void) const;

            /**
             * \brief Sets the rotation component of the transform.
             *
             * \param rotation_radians The new rotation component for the
             *        transform in the order (pitch, yaw, roll).
             */
            void set_rotation(const Vector3f &rotation);

            /**
             * \brief Sets the rotation component of the transform.
             *
             * \param pitch The new pitch for the transform in radians.
             * \param yaw The new yaw for the transform in radians.
             * \param roll The new roll for the transform in radians.
             */
            void set_rotation(float pitch, float yaw, float roll);

            /**
             * \brief Adds the given value to the transform's rotation
             *        component.
             *
             * \param rotation_radians The values in radians to add to this
             *         transform's rotation component in the order (pitch, yaw,
             *         roll).
             */
            void add_rotation(const Vector3f &rotation);

            /**
             * \brief Adds the given value to the transform's rotation
             *        component.
             *
             * \param pitch_delta The value in radians to add to the transform's
             *        pitch.
             * \param yaw_delta The value in radians to add to the transform's
             *        yaw.
             * \param roll_delta The value in radians to add to the transform's
             *        roll.
             */
            void add_rotation(float pitch_delta, float yaw_delta, float roll_delta);

            /**
             * \brief Gets the scale component of the transform.
             *
             * \return The scale component of the transform.
             */
            Vector3f get_scale(void) const;

            /**
             * \brief Sets the scale component of the transform.
             *
             * \param scale The new scale for the transform.
             */
            void set_scale(const Vector3f &scale);

            /**
             * \brief Sets the scale component of the transform.
             *
             * \param x The new x-scale for the transform.
             * \param y The new y-scale for the transform.
             * \param y The new z-scale for the transform.
             */
            void set_scale(float x, float y, float z);

            /**
             * \brief Returns an unmodifiable 4x4 matrix representation of this
             *        transform.
             *
             * \return The matrix representation.
             */
            const mat4_flat_t &as_matrix(void);

            /**
             * \brief Copys a 4x4 matrix representation of the transform into
             *        the given array.
             *
             * \param target The array to copy the matrix representation into.
             */
            void copy_matrix(mat4_flat_t &target);
    };
}
