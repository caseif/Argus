/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/math.hpp"

// module renderer
#include "argus/renderer.hpp"

#include <cmath>

namespace argus {

    Transform::Transform(void):
        translation({0, 0}),
        rotation(0),
        scale({1, 1}) {
    }

    Transform::Transform(Transform &rhs):
            translation(rhs.get_translation()),
            rotation(rhs.get_rotation()),
            scale(rhs.get_scale()) {
    }

    Transform::Transform(Transform &&rhs):
            translation(std::move(rhs.translation)),
            rotation(rhs.rotation.load()),
            scale(std::move(rhs.scale)) {
    }

    Transform::Transform(Vector2f const &translation, const float rotation, Vector2f const &scale):
            translation(translation),
            rotation(rotation),
            scale(scale) {
    }

    Transform Transform::operator +(const Transform rhs) {
        return Transform(
                this->translation + rhs.translation,
                this->rotation.load() + rhs.rotation,
                this->scale * rhs.scale
        );
    }

    Vector2f const Transform::get_translation(void) {
        this->translation_mutex.lock();
        Vector2f translation_current = this->translation;
        this->translation_mutex.unlock();

        return translation;
    }

    void Transform::set_translation(Vector2f const &translation) {
        translation_mutex.lock();
        this->translation = translation;
        translation_mutex.unlock();

        this->dirty = true;
    }

    void Transform::add_translation(Vector2f const &translation_delta) {
        translation_mutex.lock();
        this->translation += translation_delta;
        translation_mutex.unlock();

        this->dirty = true;
    }

    const float Transform::get_rotation(void) const {
        return rotation;
    }

    void Transform::set_rotation(const float rotation_radians) {
        this->rotation = rotation_radians;

        this->dirty = true;
    }

    void Transform::add_rotation(const float rotation_radians) {
        float current = this->rotation.load();
        while (!this->rotation.compare_exchange_weak(current, current + rotation_radians));
        this->dirty = true;
    }

    Vector2f const Transform::get_scale(void) {
        this->scale_mutex.lock();
        Vector2f scale_current = this->scale;
        this->scale_mutex.unlock();

        return scale_current;
    }

    void Transform::set_scale(Vector2f const &scale) {
        this->scale_mutex.lock();
        this->scale = scale;
        this->scale_mutex.unlock();

        this->dirty = true;
    }

    void Transform::to_matrix(float (&dst_arr)[16]) {
        float cos_rot = std::cos(rotation);
        float sin_rot = std::sin(rotation);

        translation_mutex.lock();
        Vector2f translation_current = translation;
        translation_mutex.unlock();

        scale_mutex.lock();
        Vector2f scale_current = scale;
        scale_mutex.unlock();

        // this is transposed from the actual matrix, since GL interprets it in column-major order
        // also, really wish C++ had a more syntactically elegant way to do this
        dst_arr[0] =  cos_rot * scale_current.x;
        dst_arr[1] =  sin_rot;
        dst_arr[2] =  0;
        dst_arr[3] =  0;
        dst_arr[4] =  -sin_rot;
        dst_arr[5] =  cos_rot * scale_current.y;
        dst_arr[6] =  0;
        dst_arr[7] =  0;
        dst_arr[8] =  0;
        dst_arr[9] =  0;
        dst_arr[10] =  1;
        dst_arr[11] =  0;
        dst_arr[12] =  translation_current.x;
        dst_arr[13] =  translation_current.y;
        dst_arr[14] =  0;
        dst_arr[15] =  1;
    }

    const bool Transform::is_dirty(void) const {
        return dirty;
    }

    void Transform::clean(void) {
        dirty = false;
    }

}