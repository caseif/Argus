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
#include "internal/render/pimpl/common/transform_3d.hpp"

#include <atomic>
#include <mutex>

#include <cmath> // IWYU pragma: keep
#include <cstring>

namespace argus {

    static AllocPool g_pimpl_pool(sizeof(pimpl_Transform3D));

    Transform3D::Transform3D(void): Transform3D({0, 0, 0}, {0, 0, 0}, {1, 1, 1}) {
    }

    Transform3D::Transform3D(const Vector3f &translation, const Vector3f &rotation, const Vector3f &scale):
            pimpl(&g_pimpl_pool.construct<pimpl_Transform3D>(translation, rotation, scale)) {
    }

    Transform3D::Transform3D(const Transform3D &rhs) noexcept: Transform3D(
            rhs.pimpl->translation,
            rhs.pimpl->rotation,
            rhs.pimpl->scale
    ) {
    }

    // for the move ctor, we just steal the pimpl
    Transform3D::Transform3D(Transform3D &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Transform3D::~Transform3D(void) {
        if (pimpl != nullptr) {
            g_pimpl_pool.free(pimpl);
        }
    }

    void Transform3D::operator=(const Transform3D &rhs) noexcept {
        pimpl->translation = rhs.pimpl->translation;
        pimpl->rotation = rhs.pimpl->rotation;
        pimpl->scale = rhs.pimpl->scale;
        pimpl->dirty = true;
        pimpl->dirty_matrix = true;
    }

    Transform3D Transform3D::operator +(const Transform3D rhs) {
        return Transform3D(
                pimpl->translation + rhs.pimpl->translation,
                pimpl->rotation + rhs.pimpl->rotation,
                pimpl->scale * rhs.pimpl->scale
        );
    }

    const Vector3f Transform3D::get_translation(void) {
        pimpl->translation_mutex.lock();
        Vector3f translation_current = pimpl->translation;
        pimpl->translation_mutex.unlock();

        return this->pimpl->translation;
    }

    void Transform3D::set_translation(const Vector3f &translation) {
        pimpl->translation_mutex.lock();
        pimpl->translation = translation;
        pimpl->translation_mutex.unlock();

        pimpl->set_dirty();
    }

    void Transform3D::set_translation(const float x, const float y, const float z) {
        this->set_translation({ x, y, z });
    }

    void Transform3D::add_translation(const Vector3f &translation_delta) {
        pimpl->translation_mutex.lock();
        pimpl->translation += translation_delta;
        pimpl->translation_mutex.unlock();

        pimpl->set_dirty();
    }

    void Transform3D::add_translation(const float x, const float y, const float z) {
        this->add_translation({ x, y, z });
    }

    const Vector3f Transform3D::get_rotation(void) const {
        return pimpl->rotation;
    }

    void Transform3D::set_rotation(const Vector3f &rotation_radians) {
        pimpl->rotation = rotation_radians;

        pimpl->set_dirty();
    }

    void Transform3D::set_rotation(const float pitch, const float yaw, const float roll) {
        this->set_rotation({ pitch, yaw, roll });
    }

    void Transform3D::add_rotation(const Vector3f &rotation_delta) {
        pimpl->translation_mutex.lock();
        pimpl->translation += rotation_delta;
        pimpl->translation_mutex.unlock();

        pimpl->set_dirty();
    }

    void Transform3D::add_rotation(const float pitch_delta, const float yaw_delta, const float roll_delta) {
        this->add_rotation(pitch_delta, yaw_delta, roll_delta);
    }

    Vector3f const Transform3D::get_scale(void) {
        pimpl->scale_mutex.lock();
        Vector3f scale_current = pimpl->scale;
        pimpl->scale_mutex.unlock();

        return scale_current;
    }

    void Transform3D::set_scale(const Vector3f &scale) {
        pimpl->scale_mutex.lock();
        pimpl->scale = scale;
        pimpl->scale_mutex.unlock();

        pimpl->set_dirty();
    }

    void Transform3D::set_scale(const float x, const float y, const float z) {
        this->set_scale({ x, y, z });
    }

    static void _compute_matrix(Transform3D &transform) {
        if (!transform.pimpl->dirty_matrix) {
            return;
        }

        float cos_p = std::cos(transform.pimpl->rotation.x);
        float sin_p = std::sin(transform.pimpl->rotation.x);
        float cos_y = std::cos(transform.pimpl->rotation.y);
        float sin_y = std::sin(transform.pimpl->rotation.y);
        float cos_r = std::cos(transform.pimpl->rotation.z);
        float sin_r = std::sin(transform.pimpl->rotation.z);

        transform.pimpl->translation_mutex.lock();
        Vector3f translation_current = transform.pimpl->translation;
        transform.pimpl->translation_mutex.unlock();

        transform.pimpl->scale_mutex.lock();
        Vector3f scale_current = transform.pimpl->scale;
        transform.pimpl->scale_mutex.unlock();

        auto dst = transform.pimpl->matrix_rep;

        //TODO: compute matrix (probably going to use glm)

        transform.pimpl->dirty_matrix = false;
    }

    const mat4_flat_t &Transform3D::as_matrix(void) {
        _compute_matrix(*this);

        return pimpl->matrix_rep;
    }

    void Transform3D::copy_matrix(mat4_flat_t target) {
        _compute_matrix(*this);

        memcpy(target, pimpl->matrix_rep, 16 * sizeof(pimpl->matrix_rep[0]));
    }

}
