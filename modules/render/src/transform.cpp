/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/transform.hpp"
#include "internal/render/pimpl/transform.hpp"

#include <atomic>
#include <mutex>
#include <new>

#include <cmath> // IWYU pragma: keep
#include <cstring>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_Transform));

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

    void Transform::operator=(Transform &rhs) {
        g_pimpl_pool.free(this->pimpl);
        new(this->pimpl) pimpl_Transform(rhs.get_translation(), rhs.get_rotation(), rhs.get_scale());
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

        pimpl->set_dirty();
    }

    void Transform::add_translation(const Vector2f &translation_delta) {
        pimpl->translation_mutex.lock();
        pimpl->translation += translation_delta;
        pimpl->translation_mutex.unlock();

        pimpl->set_dirty();
    }

    const float Transform::get_rotation(void) const {
        return pimpl->rotation;
    }

    void Transform::set_rotation(const float rotation_radians) {
        pimpl->rotation = rotation_radians;

        pimpl->set_dirty();
    }

    void Transform::add_rotation(const float rotation_radians) {
        float current = pimpl->rotation.load();
        while (!pimpl->rotation.compare_exchange_weak(current, current + rotation_radians));
        pimpl->set_dirty();
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

        pimpl->set_dirty();
    }

    static void _compute_matrix(Transform &transform) {
        if (!transform.pimpl->dirty_matrix) {
            return;
        }

        float cos_rot = std::cos(transform.pimpl->rotation);
        float sin_rot = std::sin(transform.pimpl->rotation);

        transform.pimpl->translation_mutex.lock();
        Vector2f translation_current = transform.pimpl->translation;
        transform.pimpl->translation_mutex.unlock();

        transform.pimpl->scale_mutex.lock();
        Vector2f scale_current = transform.pimpl->scale;
        transform.pimpl->scale_mutex.unlock();

        auto dst = transform.pimpl->matrix_rep;

        // this is transposed from the actual matrix, since GL interprets it in column-major order
        // also, really wish C++ had a more syntactically elegant way to do this
        dst[0] =  cos_rot * scale_current.x;
        dst[1] =  sin_rot;
        dst[2] =  0;
        dst[3] =  0;
        dst[4] =  -sin_rot;
        dst[5] =  cos_rot * scale_current.y;
        dst[6] =  0;
        dst[7] =  0;
        dst[8] =  0;
        dst[9] =  0;
        dst[10] =  1;
        dst[11] =  0;
        dst[12] =  translation_current.x;
        dst[13] =  translation_current.y;
        dst[14] =  0;
        dst[15] =  1;

        transform.pimpl->dirty_matrix = false;
    }

    const mat4_flat_t &Transform::as_matrix(void) {
        _compute_matrix(*this);

        return pimpl->matrix_rep;
    }

    void Transform::copy_matrix(mat4_flat_t target) {
        _compute_matrix(*this);

        memcpy(target, pimpl->matrix_rep, 16 * sizeof(pimpl->matrix_rep[0]));
    }

    const bool Transform::is_dirty(void) const {
        return pimpl->dirty;
    }

}
