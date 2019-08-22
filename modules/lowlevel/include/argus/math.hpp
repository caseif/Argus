#pragma once

#include <type_traits>

template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
struct Vector2 {
    T x;
    T y;

    Vector2<T> operator +(Vector2<T> const &rhs) {
        return {x + rhs.x, y + rhs.y};
    }

    Vector2<T> operator -(Vector2<T> const &rhs) {
        return {x - rhs.x, y - rhs.y};
    }

    Vector2<T> operator *(Vector2<T> const &rhs) {
        return {x * rhs.x, y * rhs.y};
    }

    Vector2<T> &operator +=(Vector2<T> const &rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vector2<T> &operator -=(Vector2<T> const &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Vector2<T> &operator *=(Vector2<T> const &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }
};

template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
struct Vector3 {
    union {
        T x;
        T r;
    };
    union {
        T y;
        T g;
    };
    union {
        T z;
        T b;
    };

    Vector3<T> operator +(Vector3<T> const &rhs) {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
    }

    Vector3<T> operator -(Vector3<T> const &rhs) {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    T operator *(Vector3<T> const &rhs) {
        return {x * rhs.x, y * rhs.y, z * rhs.z};
    }

    Vector3<T> &operator +=(Vector3<T> const &rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    Vector3<T> &operator -=(Vector3<T> const &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    Vector3<T> &operator *=(Vector3<T> const &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }
};

template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
struct Vector4 {
    union {
        T x;
        T r;
    };
    union {
        T y;
        T g;
    };
    union {
        T z;
        T b;
    };
    union {
        T w;
        T a;
    };

    Vector4<T> operator +(Vector4<T> const &rhs) {
        return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w};
    }

    Vector4<T> operator -(Vector4<T> const &rhs) {
        return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w};
    }

    Vector4<T> operator *(Vector4<T> const &rhs) {
        return {x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w};
    }

    Vector4<T> &operator +=(Vector4<T> const &rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    Vector4<T> &operator -=(Vector4<T> const &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    Vector4<T> &operator *=(Vector4<T> const &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;
        return *this;
    }
};

typedef Vector2<int> Vector2i;
typedef Vector2<unsigned int> Vector2u;
typedef Vector2<float> Vector2f;
typedef Vector2<double> Vector2d;

typedef Vector3<int> Vector3i;
typedef Vector3<unsigned int> Vector3u;
typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;

typedef Vector4<int> Vector4i;
typedef Vector4<unsigned int> Vector4u;
typedef Vector4<float> Vector4f;
typedef Vector4<double> Vector4d;
