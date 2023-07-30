/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
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

#pragma once

#include "argus/lowlevel/misc.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <type_traits>

#include <cstdint>

namespace argus {
    /**
     * \brief Represents a vector with N elements.
     *
     * \tparam N The number of elements contained by this vector.
     * \tparam T The type of element contained by this vector. This must be a number
     *           which passes std::is_arithmetic.
     */
    template<typename T, unsigned int N,
            typename = typename std::enable_if<std::is_arithmetic<T>::value, void>::type,
            typename = typename std::enable_if<(N > 0 && N < UINT32_MAX), void>::type>
    struct Vector {
        std::array<T, N> elements{};

        FieldProxy<T> x = FieldProxy<T>(elements[0]);
        FieldProxy<T> y = FieldProxy<T>(elements[1]);
        FieldProxy<T> z = FieldProxy<T>(elements[2]);
        FieldProxy<T> w = FieldProxy<T>(elements[3]);

        FieldProxy<T> r = FieldProxy<T>(elements[0]);
        FieldProxy<T> g = FieldProxy<T>(elements[1]);
        FieldProxy<T> b = FieldProxy<T>(elements[2]);
        FieldProxy<T> a = FieldProxy<T>(elements[3]);

        const T &operator[](size_t index) const {
            return elements[index];
        }

        T &operator[](size_t index) {
            return elements[index];
        }

        /**
         * \brief Performs an element-wise comparison between two vectors.
         *
         * \param rhs The vector to compare against.
         */
        bool operator==(const Vector<T, N> &rhs) const {
            return std::equal(elements.begin(), elements.end(), rhs.elements.begin(), rhs.elements.end());
        }

        /**
         * \brief Performs a negative element-wise comparison between two
         *        vectors.
         *
         * \param rhs The vector to compare against.
         */
        bool operator!=(const Vector<T, N> &rhs) const {
            return !(*this == rhs);
        }

        /**
         * \brief Performs element-wise addition with another Vector4 with the same
         *        element type, returning the result as a new Vector4.
         *
         * \param rhs The vector to add to this one.
         *
         * \return The element-wise sum of the two vectors as a new Vector4.
         */
        Vector<T, N> operator+(const Vector<T, N> &rhs) const {
            Vector<T, N> res;
            for (size_t i = 0; i < N; i++) {
                res[i] = elements[i] + rhs.elements[i];
            }
            return res;
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
        Vector<T, N> operator-(const Vector<T, N> &rhs) const {
            Vector<T, N> res;
            for (size_t i = 0; i < N; i++) {
                res[i] = elements[i] - rhs.elements[i];
            }
            return res;
        }

        /**
         * \brief Performs element-wise multiplication with another Vector4 with the
         *        same element type, returning the result as a new Vector4.
         *
         * \param rhs The vector to multiply this one by.
         *
         * \return The element-wise product of the two vectors as a new Vector4.
         */
        Vector<T, N> operator*(const Vector<T, N> &rhs) const {
            Vector<T, N> res;
            for (size_t i = 0; i < N; i++) {
                res[i] = elements[i] * rhs.elements[i];
            }
            return res;
        }

        /**
         * Multiples each element of the vector by a constant value, returning
         * the result as a new Vector2.
         *
         * @param rhs The constant value to multiply the vector by.
         *
         * @return The resultant scaled vector.
         */
        Vector<T, N> operator*(T rhs) const {
            Vector<T, N> res;
            for (size_t i = 0; i < N; i++) {
                res[i] = elements[i] * rhs;
            }
            return res;
        }

        /**
         * Divides each element of the vector by a constant value, returning
         * the result as a new Vector2.
         *
         * @param rhs The constant value to divide the vector by.
         *
         * @return The resultant scaled vector.
         */
        Vector<T, N> operator/(T rhs) const {
            Vector<T, N> res;
            for (size_t i = 0; i < N; i++) {
                res[i] = elements[i] / rhs;
            }
            return res;
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
        Vector<T, N> &operator+=(const Vector<T, N> &rhs) {
            for (size_t i = 0; i < N; i++) {
                elements[i] += rhs.elements[i];
            }
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
        Vector<T, N> &operator-=(const Vector<T, N> &rhs) {
            for (size_t i = 0; i < N; i++) {
                elements[i] -= rhs.elements[i];
            }
            return *this;
        }

        /**
         * \brief Performs in-place element-wise multiplication with another
         *        Vector.
         *
         * \param rhs The Vector to multiply this by.
         *
         * \return This Vector after being updated.
         *
         * \sa Vector::operator*
         */
        Vector<T, N> &operator*=(const Vector<T, N> &rhs) {
            for (size_t i = 0; i < N; i++) {
                elements[i] *= rhs.elements[i];
            }
            return *this;
        }

        /**
         * \brief Performs in-place multiplication against a constant value.
         *
         * @param rhs The constant value to multiply each vector element by.
         *
         * @return This Vector after being updated.
         *
         * \sa Vector::operator*
         */
        Vector<T, N> &operator*=(T rhs) {
            for (size_t i = 0; i < N; i++) {
                elements[i] = elements[i] * rhs;
            }
            return *this;
        }

        /**
         * \brief Performs in-place division against a constant value.
         *
         * @param rhs The constant value to divide each vector element by.
         *
         * @return This Vector after being updated.
         *
         * \sa Vector::operator/
         */
        Vector<T, N> &operator/=(T rhs) {
            for (size_t i = 0; i < N; i++) {
                elements[i] = elements[i] / rhs;
            }
            return *this;
        }

        template <unsigned int N2>
        operator std::enable_if_t<(N2 < N), Vector<T, N2>>(void) const {
            return Vector<T, N2>(*this);
        }

        /*template <typename... Args>
        Vector(Args... elements) : elements(elements...) {
            static_assert(sizeof...(elements) == N, "Wrong parameter count");
        }*/

        template<unsigned int N2, typename = std::enable_if<(N2 < N)>>
        Vector(const Vector<T, N2> &v) {
            std::copy(v.elements.begin(), v.elements.end(), elements.begin());
        }

        template<bool B>
        using EnableIfB = typename std::enable_if<B, int>::type;

        template <unsigned int N2 = N, EnableIfB<N2 == 4> = 0>
        Vector(T x, T y, T z, T w) : elements({ x, y, z, w }) {
        }

        template <unsigned int N2 = N, EnableIfB<N2 == 3> = 0>
        Vector(T x, T y, T z) : elements({ x, y, z }) {
        }

        template <unsigned int N2 = N, EnableIfB<N2 == 2> = 0>
        Vector(T x, T y) : elements({ x, y }) {
        }

        template <unsigned int N2 = N, EnableIfB<N2 == 1> = 0>
        Vector(T x) : elements({ x }) {
        }

        Vector(void) : elements({}) {
        }

        Vector<T, N> inverse(void) const {
            Vector<T, N> res;
            std::transform(elements.cbegin(), elements.cend(),
                    res.elements.begin(),
                    [](const auto &e) { return -e; });
            return res;
        }
    };

    /**
     * \brief Represents a vector with two elements.
     *
     * \tparam T The type of element contained by this vector. This must be an
     *         arithmetic type.
     */
    template <typename T>
    using Vector2 = Vector<T, 2>;

    /**
     * \brief Represents a vector with two elements.
     *
     * \tparam T The type of element contained by this vector. This must be an
     *         arithmetic type.
     */
    template <typename T>
    using Vector3 = Vector<T, 3>;

    /**
     * \brief Represents a vector with two elements.
     *
     * \tparam T The type of element contained by this vector. This must be an
     *         arithmetic type.
     */
    template <typename T>
    using Vector4 = Vector<T, 4>;

    /**
     * \brief Represents a vector of two `int32_t`s.
     */
    typedef Vector2<int32_t> Vector2i;

    /**
     * \brief Represents a vector of two `uint32_t`s.
     */
    typedef Vector2<uint32_t> Vector2u;

    /**
     * \brief Represents a vector of two `float`s.
     */
    typedef Vector2<float> Vector2f;

    /**
     * \brief Represents a vector of two `double`s.
     */
    typedef Vector2<double> Vector2d;

    /**
     * \brief Represents a vector of three `int32_t`s.
     */
    typedef Vector3<int32_t> Vector3i;

    /**
     * \brief Represents a vector of three `uint32_t`s.
     */
    typedef Vector3<uint32_t> Vector3u;

    /**
     * \brief Represents a vector of three `float`s.
     */
    typedef Vector3<float> Vector3f;

    /**
     * \brief Represents a vector of three `double`s.
     */
    typedef Vector3<double> Vector3d;

    /**
     * \brief Represents a vector of four `int32_t`s.
     */
    typedef Vector4<int32_t> Vector4i;

    /**
     * \brief Represents a vector of four `uint32_t`s.
     */
    typedef Vector4<uint32_t> Vector4u;

    /**
     * \brief Represents a vector of four `float`s.
     */
    typedef Vector4<float> Vector4f;

    /**
     * \brief Represents a vector of four `double`s.
     */
    typedef Vector4<double> Vector4d;
}
