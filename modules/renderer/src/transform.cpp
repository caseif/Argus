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
#include "argus/memory.hpp"

// module renderer
#include "argus/renderer.hpp"

#include <cmath>

namespace argus {

    struct pimpl_Transform {
        Vector2f translation;
        std::atomic<float> rotation;
        Vector2f scale;

        std::mutex translation_mutex;
        std::mutex scale_mutex;

        std::atomic_bool dirty;

        pimpl_Transform(const Vector2f &translation, const float rotation, const Vector2f &scale):
            translation(translation),
            rotation(rotation),
            scale(scale) {
        }
    };

    static AllocPool g_pimpl_pool(sizeof(pimpl_Transform), 1024);

    Transform::Transform(void): Transform({0, 0}, 0, {1, 1}) {
    }

    Transform::Transform(Transform &rhs): Transform(
            rhs.pimpl->translation,
            rhs.pimpl->rotation,
            rhs.pimpl->scale
    ) {
    }

    // for the move ctor, we just steal the pimpl
    Transform::Transform(Transform &&rhs):
            pimpl(rhs.pimpl) {
    }

    Transform::Transform(const Vector2f &translation, const float rotation, const Vector2f &scale):
            pimpl(&g_pimpl_pool.construct<pimpl_Transform>(translation, rotation, scale)) {
    }

    Transform::~Transform(void) {
        g_pimpl_pool.free(pimpl);
    }

    Transform Transform::operator +(const Transform rhs) {
        return Transform(
                pimpl->translation + rhs.pimpl->translation,
                pimpl->rotation.load() + rhs.pimpl->rotation,
                pimpl->scale * rhs.pimpl->scale
        );
    }

    Vector2f const Transform::get_translation(void) {
        pimpl->translation_mutex.lock();
        Vector2f translation_current = pimpl->translation;
        pimpl->translation_mutex.unlock();

        return this->pimpl->translation;
    }

    void Transform::set_translation(const Vector2f &translation) {
        pimpl->translation_mutex.lock();
        pimpl->translation = translation;
        pimpl->translation_mutex.unlock();

        pimpl->dirty = true;
    }

    void Transform::add_translation(const Vector2f &translation_delta) {
        pimpl->translation_mutex.lock();
        pimpl->translation += translation_delta;
        pimpl->translation_mutex.unlock();

        pimpl->dirty = true;
    }

    const float Transform::get_rotation(void) const {
        return pimpl->rotation;
    }

    void Transform::set_rotation(const float rotation_radians) {
        pimpl->rotation = rotation_radians;

        pimpl->dirty = true;
    }

    void Transform::add_rotation(const float rotation_radians) {
        float current = pimpl->rotation.load();
        while (!pimpl->rotation.compare_exchange_weak(current, current + rotation_radians));
        pimpl->dirty = true;
    }

    Vector2f const Transform::get_scale(void) {
        pimpl->scale_mutex.lock();
        Vector2f scale_current = pimpl->scale;
        pimpl->scale_mutex.unlock();

        return scale_current;
    }

    void Transform::set_scale(const Vector2f &scale) {
        pimpl->scale_mutex.lock();
        pimpl->scale = scale;
        pimpl->scale_mutex.unlock();

        pimpl->dirty = true;
    }

    void Transform::to_matrix(float (&dst_arr)[16]) {
        float cos_rot = std::cos(pimpl->rotation);
        float sin_rot = std::sin(pimpl->rotation);

        pimpl->translation_mutex.lock();
        Vector2f translation_current = pimpl->translation;
        pimpl->translation_mutex.unlock();

        pimpl->scale_mutex.lock();
        Vector2f scale_current = pimpl->scale;
        pimpl->scale_mutex.unlock();

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
        return pimpl->dirty;
    }

    void Transform::clean(void) {
        pimpl->dirty = false;
    }

}