#pragma once

#include <type_traits>

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
    Vector2<T> operator +(Vector2<T> const &rhs) {
        return {x + rhs.x, y + rhs.y};
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
    Vector2<T> operator -(Vector2<T> const &rhs) {
        return {x - rhs.x, y - rhs.y};
    }

    /**
     * \brief Performs element-wise multiplication with another Vector2 with the
     *        same element type, returning the result as a new Vector2.
     *
     * \param rhs The vector to multiply this one by.
     *
     * \return The element-wise product of the two vectors as a new Vector2.
     */
    Vector2<T> operator *(Vector2<T> const &rhs) {
        return {x * rhs.x, y * rhs.y};
    }

    /**
     * \brief Performs in-place element-wise addition with another Vector2.
     *
     * \return This Vector2 after being updated.
     *
     * \sa Vector2::operator+
     */
    Vector2<T> &operator +=(Vector2<T> const &rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    /**
     * \brief Performs in-place element-wise subtraction with another Vector2.
     *
     * \return This Vector2 after being updated.
     *
     * \sa Vector2::operator-
     */
    Vector2<T> &operator -=(Vector2<T> const &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    /**
     * \brief Performs in-place element-wise multiplication with another
     *        Vector2.
     *
     * \return This Vector2 after being updated.
     *
     * \sa Vector2::operator*
     */
    Vector2<T> &operator *=(Vector2<T> const &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
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
     * \return The element-wise sum of the two vectors as a new Vector2.
     */
    Vector3<T> operator +(Vector3<T> const &rhs) {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
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
     *         Vector2.
     */
    Vector3<T> operator -(Vector3<T> const &rhs) {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    /**
     * \brief Performs element-wise multiplication with another Vector3 with the
     *        same element type, returning the result as a new Vector3.
     *
     * \param rhs The vector to multiply this one by.
     *
     * \return The element-wise product of the two vectors as a new Vector2.
     */
    T operator *(Vector3<T> const &rhs) {
        return {x * rhs.x, y * rhs.y, z * rhs.z};
    }

    /**
     * \brief Performs in-place element-wise addition with another Vector3.
     *
     * \return This Vector3 after being updated.
     *
     * \sa Vector3::operator+
     */
    Vector3<T> &operator +=(Vector3<T> const &rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    /**
     * \brief Performs in-place element-wise subtraction with another Vector3.
     *
     * \return This Vector3 after being updated.
     *
     * \sa Vector3::operator-
     */
    Vector3<T> &operator -=(Vector3<T> const &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    /**
     * \brief Performs in-place element-wise multiplication with another
     *        Vector3.
     *
     * \return This Vector3 after being updated.
     *
     * \sa Vector3::operator*
     */
    Vector3<T> &operator *=(Vector3<T> const &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }
};

/**
 * \brief Represents a vector with four elements.
 *
 * \tparam T The type of element contained by this vector. This must be a number
 *           which passes std::is_arithmetic.
 */
template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
struct Vector4 {
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
    Vector4<T> operator +(Vector4<T> const &rhs) {
        return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w};
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
    Vector4<T> operator -(Vector4<T> const &rhs) {
        return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w};
    }

    /**
     * \brief Performs element-wise multiplication with another Vector4 with the
     *        same element type, returning the result as a new Vector4.
     *
     * \param rhs The vector to multiply this one by.
     *
     * \return The element-wise product of the two vectors as a new Vector4.
     */
    Vector4<T> operator *(Vector4<T> const &rhs) {
        return {x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w};
    }

    /**
     * \brief Performs in-place element-wise addition with another Vector4.
     *
     * \return This Vector4 after being updated.
     *
     * \sa Vector4::operator+
     */
    Vector4<T> &operator +=(Vector4<T> const &rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    /**
     * \brief Performs in-place element-wise subtraction with another Vector4.
     *
     * \return This Vector4 after being updated.
     *
     * \sa Vector4::operator-
     */
    Vector4<T> &operator -=(Vector4<T> const &rhs) {
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
     * \return This Vector4 after being updated.
     *
     * \sa Vector4::operator*
     */
    Vector4<T> &operator *=(Vector4<T> const &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;
        return *this;
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
