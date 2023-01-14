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

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

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
            g_pimpl_pool.destroy(pimpl);
        }
    }

    Transform3D &Transform3D::operator =(const Transform3D &rhs) noexcept {
        pimpl->translation_mutex.lock();
        pimpl->rotation_mutex.lock();
        pimpl->scale_mutex.lock();

        pimpl->translation = rhs.pimpl->translation;
        pimpl->rotation = rhs.pimpl->rotation;
        pimpl->scale = rhs.pimpl->scale;
        pimpl->dirty = (&rhs != this) || this->pimpl->dirty;
        pimpl->dirty_matrix = (&rhs != this) || this->pimpl->dirty_matrix;
        
        pimpl->scale_mutex.unlock();
        pimpl->rotation_mutex.unlock();
        pimpl->translation_mutex.unlock();

        return *this;
    }

    Transform3D Transform3D::operator +(const Transform3D &rhs) const {
        return Transform3D(
                pimpl->translation + rhs.pimpl->translation,
                pimpl->rotation + rhs.pimpl->rotation,
                pimpl->scale * rhs.pimpl->scale
        );
    }

    Vector3f Transform3D::get_translation(void) const {
        pimpl->translation_mutex.lock();
        Vector3f translation_current = pimpl->translation;
        pimpl->translation_mutex.unlock();

        return translation_current;
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

    Vector3f Transform3D::get_rotation(void) const {
        return pimpl->rotation;
    }

    void Transform3D::set_rotation(const Vector3f &rotation_radians) {
        pimpl->rotation_mutex.lock();
        pimpl->rotation = rotation_radians;
        pimpl->rotation_mutex.unlock();

        pimpl->set_dirty();
    }

    void Transform3D::set_rotation(const float pitch, const float yaw, const float roll) {
        this->set_rotation({ pitch, yaw, roll });
    }

    void Transform3D::add_rotation(const Vector3f &rotation_delta) {
        pimpl->rotation_mutex.lock();
        pimpl->rotation += rotation_delta;
        pimpl->rotation_mutex.unlock();

        pimpl->set_dirty();
    }

    void Transform3D::add_rotation(const float pitch_delta, const float yaw_delta, const float roll_delta) {
        this->add_rotation(Vector3f { pitch_delta, yaw_delta, roll_delta });
    }

    Vector3f Transform3D::get_scale(void) const {
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

        /*float cos_p = std::cos(transform.pimpl->rotation.x);
        float sin_p = std::sin(transform.pimpl->rotation.x);
        float cos_y = std::cos(transform.pimpl->rotation.y);
        float sin_y = std::sin(transform.pimpl->rotation.y);
        float cos_r = std::cos(transform.pimpl->rotation.z);
        float sin_r = std::sin(transform.pimpl->rotation.z);

        transform.pimpl->translation_mutex.lock();
        Vector3f trans_current = transform.pimpl->translation;
        transform.pimpl->translation_mutex.unlock();

        transform.pimpl->rotation_mutex.lock();
        Vector3f rot_current = transform.pimpl->rotation;
        transform.pimpl->rotation_mutex.unlock();

        transform.pimpl->scale_mutex.lock();
        Vector3f scale_current = transform.pimpl->scale;
        transform.pimpl->scale_mutex.unlock();

        auto dst = transform.pimpl->matrix_rep;*/

        //TODO: compute matrix (probably going to use glm)

        transform.pimpl->dirty_matrix = false;
    }

    const Matrix4 &Transform3D::as_matrix(void) {
        _compute_matrix(*this);

        return pimpl->matrix_rep;
    }

    void Transform3D::copy_matrix(Matrix4 &target) {
        _compute_matrix(*this);

        target = pimpl->matrix_rep;
    }
}
