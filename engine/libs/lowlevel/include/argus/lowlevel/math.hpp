/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file argus/lowlevel/math.hpp
 *
 * Various mathematics utility functions and classes.
 */

#pragma once

#include <string>
#include <type_traits>

namespace argus {
    #pragma pack(push, 1)
    struct Matrix4Row {
        float data[4];

        float &operator[](int i) {
            return data[i];
        }

        float operator[](int i) const {
            return data[i];
        }

        Matrix4Row(float a, float b, float c, float d) {
            data[0] = a;
            data[1] = b;
            data[2] = c;
            data[3] = d;
        }

        Matrix4Row(void): Matrix4Row(0, 0, 0, 0) {
        }
    };

    struct Matrix4 {
        union {
            float data[16];
            Matrix4Row rows[4];
        };

        Matrix4(Matrix4Row a, Matrix4Row b, Matrix4Row c, Matrix4Row d) {
            rows[0] = a;
            rows[1] = b;
            rows[2] = c;
            rows[3] = d;
        }

        Matrix4(void): Matrix4({}, {}, {}, {}) {
        }

        Matrix4Row &operator[](int i) {
            return rows[i];
        }

        const Matrix4Row &operator[](int i) const {
            return rows[i];
        }

        static Matrix4 identity() {
            return {
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 0, 1, 0},
                {0, 0, 0, 1}
            };
        }
    };
    #pragma pack(pop)

    typedef float mat4_flat_t[16];

    std::string mat4_to_str(Matrix4 matrix);

    std::string mat4_to_str(mat4_flat_t matrix);

    /**
     * \brief Represents a vector with four elements.
     *
     * \tparam T The type of element contained by this vector. This must be a number
     *           which passes std::is_arithmetic.
     */
    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    struct Vector4 {
        /**
         * \brief Union containing the first element of the vector.
         */
        union {
            /**
             * \brief The first element of this vector.
             */
            T x;
            /**
             * \brief The first element of this vector, aliased as the red channel
             *        of an RGBA value.
             */
            T r;
        };
        /**
         * \brief Union containing the second element of the vector.
         */
        union {
            /**
             * \brief The second element of this vector.
             */
            T y;
            /**
             * \brief The second element of this vector, aliased as the green
             *        channel of an RGBA value.
             */
            T g;
        };
        /**
         * \brief Union containing the third element of the vector.
         */
        union {
            /**
             * \brief The third element of this vector.
             */
            T z;
            /**
             * \brief The third element of this vector, aliased as the blue channel
             *        of an RGBA value.
             */
            T b;
        };
        /**
         * \brief Union containing the fourth element of the vector.
         */
        union {
            /**
             * \brief The fourth element of this vector.
             */
            T w;
            /**
             * \brief The fourth element of this vector, aliased as the alpha
             *        channel of an RGBA value.
             */
            T a;
        };

        /**
         * \brief Performs element-wise addition with another Vector4 with the same
         *        element type, returning the result as a new Vector4.
         *
         * \param rhs The vector to add to this one.
         *
         * \return The element-wise sum of the two vectors as a new Vector4.
         */
        Vector4<T> operator +(const Vector4<T> &rhs) {
            return Vector4<T>(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
        }

        /**
         * \brief Performs element-wise subtraction with another Vector4 with the
         *        same element type, returning the result as a new Vector4.
         *
         * Each element of the parameter is subtracted from the respective element
         * of this one.
         *
         * \param rhs The vector to subtract from this one.
         *
         * \return The element-wise difference between the two vectors as a new
         *         Vector4.
         */
        Vector4<T> operator -(const Vector4<T> &rhs) {
            return Vector4<T>(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
        }

        /**
         * \brief Performs element-wise multiplication with another Vector4 with the
         *        same element type, returning the result as a new Vector4.
         *
         * \param rhs The vector to multiply this one by.
         *
         * \return The element-wise product of the two vectors as a new Vector4.
         */
        Vector4<T> operator *(const Vector4<T> &rhs) {
            return Vector4<T>(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
        }

        /**
         * \brief Performs in-place element-wise addition with another Vector4.
         *
         * \param rhs The Vector4 to add to this.
         *
         * \return This Vector4 after being updated.
         *
         * \sa Vector4::operator+
         */
        Vector4<T> &operator +=(const Vector4<T> &rhs) {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            w += rhs.w;
            return *this;
        }

        /**
         * \brief Performs in-place element-wise subtraction with another Vector4.
         *
         * \param rhs The Vector4 to subtract from this.
         *
         * \return This Vector4 after being updated.
         *
         * \sa Vector4::operator-
         */
        Vector4<T> &operator -=(const Vector4<T> &rhs) {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            w -= rhs.w;
            return *this;
        }

        /**
         * \brief Performs in-place element-wise multiplication with another
         *        Vector4.
         *
         * \param rhs The Vector3 to multiply this by.
         *
         * \return This Vector4 after being updated.
         *
         * \sa Vector4::operator*
         */
        Vector4<T> &operator *=(const Vector4<T> &rhs) {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            w *= rhs.w;
            return *this;
        }

        Vector4(T x, T y, T z, T w):
                x(x),
                y(y),
                z(z),
                w(w) {
        }

        Vector4(void): Vector4<T>(0, 0, 0, 0) {
        }

        Vector4<T> inverse(void) const {
            return { -x, -y, -z, -w };
        }
    };

    /**
     * \brief Represents a vector with three elements.
     *
     * \tparam T The type of element contained by this vector. This must be a number
     *           which passes std::is_arithmetic.
     */
    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    struct Vector3 {
        /**
         * \brief Union containing the first element of the vector.
         */
        union {
            /**
             * \brief The first element of this vector.
             */
            T x;
            /**
             * \brief The first element of this vector, aliased as the red channel
             *        of an RGB value.
             */
            T r;
        };
        /**
         * \brief Union containing the second element of the vector.
         */
        union {
            /**
             * \brief The second element of this vector.
             */
            T y;
            /**
             * \brief The second element of this vector, aliased as the green channel
             *        of an RGB value.
             */
            T g;
        };
        /**
         * \brief Union containing the third element of the vector.
         */
        union {
            /**
             * \brief The third element of this vector.
             */
            T z;
            /**
             * \brief The third element of this vector, aliased as the blue channel
             *        of an RGB value.
             */
            T b;
        };

        /**
         * \brief Performs element-wise addition with another Vector3 with the same
         *        element type, returning the result as a new Vector3.
         *
         * \param rhs The vector to add to this one.
         *
         * \return The element-wise sum of the two vectors as a new Vector3.
         */
        Vector3<T> operator +(const Vector3<T> &rhs) {
            return Vector3<T>(x + rhs.x, y + rhs.y, z + rhs.z);
        }

        /**
         * \brief Performs element-wise subtraction with another Vector3 with the
         *        same element type, returning the result as a new Vector3.
         *
         * Each element of the parameter is subtracted from the respective element
         * of this one.
         *
         * \param rhs The vector to subtract from this one.
         *
         * \return The element-wise difference between the two vectors as a new
         *         Vector3.
         */
        Vector3<T> operator -(const Vector3<T> &rhs) {
            return Vector3<T>(x - rhs.x, y - rhs.y, z - rhs.z);
        }

        /**
         * \brief Performs element-wise multiplication with another Vector3 with the
         *        same element type, returning the result as a new Vector3.
         *
         * \param rhs The vector to multiply this one by.
         *
         * \return The element-wise product of the two vectors as a new Vector3.
         */
        Vector3<T> operator *(const Vector3<T> &rhs) {
            return Vector3<T>(x * rhs.x, y * rhs.y, z * rhs.z);
        }

        /**
         * \brief Performs in-place element-wise addition with another Vector3.
         *
         * \param rhs The Vector3 to add to this.
         *
         * \return This Vector3 after being updated.
         *
         * \sa Vector3::operator+
         */
        Vector3<T> &operator +=(const Vector3<T> &rhs) {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }

        /**
         * \brief Performs in-place element-wise subtraction with another Vector3.
         *
         * \param rhs The Vector3 to subtract from this.
         *
         * \return This Vector3 after being updated.
         *
         * \sa Vector3::operator-
         */
        Vector3<T> &operator -=(const Vector3<T> &rhs) {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }

        /**
         * \brief Performs in-place element-wise multiplication with another
         *        Vector3.
         *
         * \param rhs The Vector3 to multiply this by.
         *
         * \return This Vector3 after being updated.
         *
         * \sa Vector3::operator*
         */
        Vector3<T> &operator *=(const Vector3<T> &rhs) {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            return *this;
        }

        //NOLINTNEXTLINE(google-explicit-constructor)
        operator Vector4<T>() const {
            return Vector4<T>(this->x, this->y, this->z, 1);
        }

        Vector3(T x, T y, T z):
                x(x),
                y(y),
                z(z) {
        }

        Vector3(void): Vector3<T>(0, 0, 0) {
        }

        Vector3<T> inverse(void) const {
            return { -x, -y, -z };
        }
    };

    /**
     * \brief Represents a vector with two elements.
     *
     * \tparam T The type of element contained by this vector. This must be a number
     *           which passes std::is_arithmetic.
     */
    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    struct Vector2 {
        /**
         * \brief The first element of the vector.
         */
        T x;
        /**
         * \brief The second element of the vector.
         */
        T y;

        /**
         * \brief Performs element-wise addition with another Vector2 with the same
         *        element type, returning the result as a new Vector2.
         *
         * \param rhs The vector to add to this one.
         *
         * \return The element-wise sum of the two vectors as a new Vector2.
         */
        Vector2<T> operator +(const Vector2<T> &rhs) {
            return Vector2<T>(x + rhs.x, y + rhs.y);
        }

        /**
         * \brief Performs element-wise subtraction with another Vector2 with the
         *        same element type, returning the result as a new Vector2.
         *
         * Each element of the parameter is subtracted from the respective element
         * of this one.
         *
         * \param rhs The vector to subtract from this one.
         *
         * \return The element-wise difference between the two vectors as a new
         *         Vector2.
         */
        Vector2<T> operator -(const Vector2<T> &rhs) {
            return Vector2<T>(x - rhs.x, y - rhs.y);
        }

        /**
         * \brief Performs element-wise multiplication with another Vector2 with the
         *        same element type, returning the result as a new Vector2.
         *
         * \param rhs The vector to multiply this one by.
         *
         * \return The element-wise product of the two vectors as a new Vector2.
         */
        Vector2<T> operator *(const Vector2<T> &rhs) {
            return Vector2<T>(x * rhs.x, y * rhs.y);
        }

        /**
         * \brief Performs in-place element-wise addition with another Vector2.
         *
         * \param rhs The Vector2 to add to this.
         *
         * \return This Vector2 after being updated.
         *
         * \sa Vector2::operator+
         */
        Vector2<T> &operator +=(const Vector2<T> &rhs) {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        /**
         * \brief Performs in-place element-wise subtraction with another Vector2.
         *
         * \param rhs The Vector2 to subtract from this.
         *
         * \return This Vector2 after being updated.
         *
         * \sa Vector2::operator-
         */
        Vector2<T> &operator -=(const Vector2<T> &rhs) {
            x -= rhs.x;
            y -= rhs.y;
            return *this;
        }

        /**
         * \brief Performs in-place element-wise multiplication with another
         *        Vector2.
         *
         * \param rhs The Vector2 to multiply this by.
         *
         * \return This Vector2 after being updated.
         *
         * \sa Vector2::operator*
         */
        Vector2<T> &operator *=(const Vector2<T> &rhs) {
            x *= rhs.x;
            y *= rhs.y;
            return *this;
        }

        //NOLINTNEXTLINE(google-explicit-constructor)
        operator Vector3<T>() const {
            return Vector3<T>(this->x, this->y, 0);
        }

        //NOLINTNEXTLINE(google-explicit-constructor)
        operator Vector4<T>() const {
            return Vector4<T>(this->x, this->y, 0, 1);
        }

        Vector2(T x, T y):
                x(x),
                y(y) {
        }

        Vector2(void): Vector2<T>(0, 0) {
        }

        Vector2<T> inverse(void) const {
            return { -x, -y };
        }
    };

    /**
     * \brief Represents a vector of two `int`s.
     */
    typedef Vector2<int> Vector2i;

    /**
     * \brief Represents a vector of two `unsigned int`s.
     */
    typedef Vector2<unsigned int> Vector2u;

    /**
     * \brief Represents a vector of two `float`s.
     */
    typedef Vector2<float> Vector2f;

    /**
     * \brief Represents a vector of two `double`s.
     */
    typedef Vector2<double> Vector2d;

    /**
     * \brief Represents a vector of three `int`s.
     */
    typedef Vector3<int> Vector3i;

    /**
     * \brief Represents a vector of three `unsigned int`s.
     */
    typedef Vector3<unsigned int> Vector3u;

    /**
     * \brief Represents a vector of three `float`s.
     */
    typedef Vector3<float> Vector3f;

    /**
     * \brief Represents a vector of three `double`s.
     */
    typedef Vector3<double> Vector3d;

    /**
     * \brief Represents a vector of four `int`s.
     */
    typedef Vector4<int> Vector4i;

    /**
     * \brief Represents a vector of four `unsigned int`s.
     */
    typedef Vector4<unsigned int> Vector4u;

    /**
     * \brief Represents a vector of four `float`s.
     */
    typedef Vector4<float> Vector4f;

    /**
     * \brief Represents a vector of four `double`s.
     */
    typedef Vector4<double> Vector4d;

    struct ScreenSpace {
        float left;
        float right;
        float top;
        float bottom;

        ScreenSpace(float left, float right, float top, float bottom):
            left(left),
            right(right),
            top(top),
            bottom(bottom) {
        }
    };

    /**
     * \brief Controls how the screen space is scaled with respect to window
     *        aspect ratio.
     *
     * When configured as any value other than `None`, either the horizontal or
     * vertical axis will be "normalized" while the other is scaled. The normal
     * axis will maintain the exact bounds requested by the provided screen
     * space configuration, while the other will have its bounds changed such
     * that the aspect ratio of the screen space matches that of the window.
     * If the window is resized, the screen space will be updated in tandem.
     */
    enum class ScreenSpaceScaleMode {
        /**
         * \brief Normalizes the screen space dimension with the minimum range.
         *
         * The bounds of the smaller window dimension will be exactly as
         * configured. Meanwhile, the bounds of the larger dimension will be
         * "extended" beyond what they would be in a square (1:1 aspect ratio)
         * window such that regions become visible which would otherwise not be
         * in the square window.
         *
         * For example, a typical computer monitor is wider than it is tall, so
         * given this mode, the bounds of the screen space of a fullscreen
         * window would be preserved in the vertical dimension, while the bounds
         * in the horizontal dimension would be larger than usual (+/- 1.778 on
         * a 16:9 monitor, exactly the ratio of the monitor dimensions).
         *
         * Meanwhile, a phone screen (held upright) is taller than it is wide,
         * so it would see the bounds of the screen space extended in the
         * vertical dimension instead.
         */
        NormalizeMinDimension,
        /**
         * \brief Normalizes the screen space dimension with the maximum range.
         *
         * This is effectively the inverse of `NormalizedMinDimension`. The
         * bounds of the screen space are preserved on the larger dimension, and
         * "shrunk" on the smaller one. This effectively hides regions that
         * would be visible in a square window.
         */
        NormalizeMaxDimension,
        /**
         * \brief Normalizes the vertical screen space dimension.
         *
         * This invariably normalizes the vertical dimension of the screen
         * space regardless of which dimension is larger. The horizontal
         * dimension is grown or shrunk depending on the aspect ratio of the
         * window.
         */
        NormalizeVertical,
        /**
         * \brief Normalizes the horizontal screen space dimension.
         *
         * This invariably normalizes the horizontal dimension of the screen
         * space regardless of which dimension is larger. The vertical
         * dimension is grown or shrunk depending on the aspect ratio of the
         * window.
         */
        NormalizeHorizontal,
        /**
         * \brief Does not normalize screen space with respect to aspect ratio.
         *
         * Given an aspect ratio other than 1:1, the contents of the window will
         * be stretched in one dimension or the other depending on which is
         * larger.
         */
        None
    };

    void multiply_matrices(const Matrix4 &a, const Matrix4 &b, Matrix4 &res);

    void multiply_matrices(Matrix4 &a, const Matrix4 &b);

    Vector4f multiply_matrix_and_vector(const Vector4f &vec, const Matrix4 &mat);

    void transpose_matrix(Matrix4 &mat);
}
