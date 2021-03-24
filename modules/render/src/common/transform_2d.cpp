/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/common/transform.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"

#include <atomic>
#include <mutex>

#include <cmath> // IWYU pragma: keep
#include <cstring>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_Transform2D));

    Transform2D::Transform2D(void): Transform2D({0, 0}, 0, {1, 1}) {
    }

    Transform2D::Transform2D(const Vector2f &translation, const float rotation, const Vector2f &scale):
            pimpl(&g_pimpl_pool.construct<pimpl_Transform2D>(translation, rotation, scale)) {
    }

    Transform2D::Transform2D(const Transform2D &rhs) noexcept: Transform2D(
            rhs.pimpl->translation,
            rhs.pimpl->rotation,
            rhs.pimpl->scale
    ) {
    }

    // for the move ctor, we just steal the pimpl
    Transform2D::Transform2D(Transform2D &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Transform2D::~Transform2D(void) {
        if (pimpl != nullptr) {
            g_pimpl_pool.free(pimpl);
        }
    }

    void Transform2D::operator=(const Transform2D &rhs) noexcept {
        pimpl->translation = rhs.pimpl->translation;
        pimpl->rotation.store(rhs.pimpl->rotation);
        pimpl->scale = rhs.pimpl->scale;
        pimpl->dirty = true;
        pimpl->dirty_matrix = true;
    }

    Transform2D Transform2D::operator +(const Transform2D rhs) {
        return Transform2D(
                pimpl->translation + rhs.pimpl->translation,
                pimpl->rotation.load() + rhs.pimpl->rotation,
                pimpl->scale * rhs.pimpl->scale
        );
    }

    Vector2f const Transform2D::get_translation(void) {
        pimpl->translation_mutex.lock();
        Vector2f translation_current = pimpl->translation;
        pimpl->translation_mutex.unlock();

        return this->pimpl->translation;
    }

    void Transform2D::set_translation(const Vector2f &translation) {
        pimpl->translation_mutex.lock();
        pimpl->translation = translation;
        pimpl->translation_mutex.unlock();

        pimpl->set_dirty();
    }

    void Transform2D::set_translation(const float x, const float y) {
        this->set_translation({ x, y });
    }

    void Transform2D::add_translation(const Vector2f &translation_delta) {
        pimpl->translation_mutex.lock();
        pimpl->translation += translation_delta;
        pimpl->translation_mutex.unlock();

        pimpl->set_dirty();
    }

    void Transform2D::add_translation(const float x, const float y) {
        this->add_translation({ x, y });
    }

    const float Transform2D::get_rotation(void) const {
        return pimpl->rotation;
    }

    void Transform2D::set_rotation(const float rotation_radians) {
        pimpl->rotation = rotation_radians;

        pimpl->set_dirty();
    }

    void Transform2D::add_rotation(const float rotation_radians) {
        float current = pimpl->rotation.load();
        float updated = fmod(current + rotation_radians, 2 * M_PI);
        while (!pimpl->rotation.compare_exchange_weak(current, updated));
        pimpl->set_dirty();
    }

    Vector2f const Transform2D::get_scale(void) {
        pimpl->scale_mutex.lock();
        Vector2f scale_current = pimpl->scale;
        pimpl->scale_mutex.unlock();

        return scale_current;
    }

    void Transform2D::set_scale(const Vector2f &scale) {
        pimpl->scale_mutex.lock();
        pimpl->scale = scale;
        pimpl->scale_mutex.unlock();

        pimpl->set_dirty();
    }

    void Transform2D::set_scale(const float x, const float y) {
        this->set_scale({ x, y });
    }

    static void _compute_matrix(Transform2D &transform) {
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

    const mat4_flat_t &Transform2D::as_matrix(void) {
        _compute_matrix(*this);

        return pimpl->matrix_rep;
    }

    void Transform2D::copy_matrix(mat4_flat_t target) {
        _compute_matrix(*this);

        memcpy(target, pimpl->matrix_rep, 16 * sizeof(pimpl->matrix_rep[0]));
    }

}
