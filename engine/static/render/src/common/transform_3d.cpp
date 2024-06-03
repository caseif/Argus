/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#if defined(_MSC_VER) || defined(__MINGW32__)
#define _USE_MATH_DEFINES
#endif

#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/render/common/transform.hpp"
#include "internal/render/pimpl/common/transform_3d.hpp"

#include <cmath> // IWYU pragma: keep

namespace argus {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_Transform3D));

    Transform3D::Transform3D(void):
        Transform3D({ 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }) {
    }

    Transform3D::Transform3D(const Vector3f &translation, const Vector3f &rotation, const Vector3f &scale):
        m_pimpl(&g_pimpl_pool.construct<pimpl_Transform3D>(translation, rotation, scale)) {
    }

    Transform3D::Transform3D(const Transform3D &rhs) noexcept:
        Transform3D(rhs.m_pimpl->translation, rhs.m_pimpl->rotation, rhs.m_pimpl->scale) {
    }

    // for the move ctor, we just steal the m_pimpl
    Transform3D::Transform3D(Transform3D &&rhs) noexcept:
            m_pimpl(rhs.m_pimpl) {
        rhs.m_pimpl = nullptr;
    }

    Transform3D::~Transform3D(void) {
        if (m_pimpl != nullptr) {
            g_pimpl_pool.destroy(m_pimpl);
        }
    }

    Transform3D &Transform3D::operator=(const Transform3D &rhs) noexcept {
        m_pimpl->translation_mutex.lock();
        m_pimpl->rotation_mutex.lock();
        m_pimpl->scale_mutex.lock();

        m_pimpl->translation = rhs.m_pimpl->translation;
        m_pimpl->rotation = rhs.m_pimpl->rotation;
        m_pimpl->scale = rhs.m_pimpl->scale;
        m_pimpl->dirty = (&rhs != this) || this->m_pimpl->dirty;
        m_pimpl->dirty_matrix = (&rhs != this) || this->m_pimpl->dirty_matrix;

        m_pimpl->scale_mutex.unlock();
        m_pimpl->rotation_mutex.unlock();
        m_pimpl->translation_mutex.unlock();

        return *this;
    }

    Transform3D Transform3D::operator+(const Transform3D &rhs) const {
        return Transform3D(
                m_pimpl->translation + rhs.m_pimpl->translation,
                m_pimpl->rotation + rhs.m_pimpl->rotation,
                m_pimpl->scale * rhs.m_pimpl->scale
        );
    }

    Vector3f Transform3D::get_translation(void) const {
        m_pimpl->translation_mutex.lock();
        Vector3f translation_current = m_pimpl->translation;
        m_pimpl->translation_mutex.unlock();

        return translation_current;
    }

    void Transform3D::set_translation(const Vector3f &translation) {
        m_pimpl->translation_mutex.lock();
        m_pimpl->translation = translation;
        m_pimpl->translation_mutex.unlock();

        m_pimpl->set_dirty();
    }

    void Transform3D::set_translation(const float x, const float y, const float z) {
        this->set_translation({ x, y, z });
    }

    void Transform3D::add_translation(const Vector3f &translation_delta) {
        m_pimpl->translation_mutex.lock();
        m_pimpl->translation += translation_delta;
        m_pimpl->translation_mutex.unlock();

        m_pimpl->set_dirty();
    }

    void Transform3D::add_translation(const float x, const float y, const float z) {
        this->add_translation({ x, y, z });
    }

    Vector3f Transform3D::get_rotation(void) const {
        return m_pimpl->rotation;
    }

    void Transform3D::set_rotation(const Vector3f &rotation_radians) {
        m_pimpl->rotation_mutex.lock();
        m_pimpl->rotation = rotation_radians;
        m_pimpl->rotation_mutex.unlock();

        m_pimpl->set_dirty();
    }

    void Transform3D::set_rotation(const float pitch, const float yaw, const float roll) {
        this->set_rotation({ pitch, yaw, roll });
    }

    void Transform3D::add_rotation(const Vector3f &rotation_delta) {
        m_pimpl->rotation_mutex.lock();
        m_pimpl->rotation += rotation_delta;
        m_pimpl->rotation_mutex.unlock();

        m_pimpl->set_dirty();
    }

    void Transform3D::add_rotation(const float pitch_delta, const float yaw_delta, const float roll_delta) {
        this->add_rotation(Vector3f { pitch_delta, yaw_delta, roll_delta });
    }

    Vector3f Transform3D::get_scale(void) const {
        m_pimpl->scale_mutex.lock();
        Vector3f scale_current = m_pimpl->scale;
        m_pimpl->scale_mutex.unlock();

        return scale_current;
    }

    void Transform3D::set_scale(const Vector3f &scale) {
        m_pimpl->scale_mutex.lock();
        m_pimpl->scale = scale;
        m_pimpl->scale_mutex.unlock();

        m_pimpl->set_dirty();
    }

    void Transform3D::set_scale(const float x, const float y, const float z) {
        this->set_scale({ x, y, z });
    }

    static void _compute_matrix(Transform3D &transform) {
        if (!transform.m_pimpl->dirty_matrix) {
            return;
        }

        /*float cos_p = std::cos(transform.m_pimpl->rotation.x);
        float sin_p = std::sin(transform.m_pimpl->rotation.x);
        float cos_y = std::cos(transform.m_pimpl->rotation.y);
        float sin_y = std::sin(transform.m_pimpl->rotation.y);
        float cos_r = std::cos(transform.m_pimpl->rotation.z);
        float sin_r = std::sin(transform.m_pimpl->rotation.z);

        transform.m_pimpl->translation_mutex.lock();
        Vector3f trans_current = transform.m_pimpl->translation;
        transform.m_pimpl->translation_mutex.unlock();

        transform.m_pimpl->rotation_mutex.lock();
        Vector3f rot_current = transform.m_pimpl->rotation;
        transform.m_pimpl->rotation_mutex.unlock();

        transform.m_pimpl->scale_mutex.lock();
        Vector3f scale_current = transform.m_pimpl->scale;
        transform.m_pimpl->scale_mutex.unlock();

        auto dst = transform.m_pimpl->matrix_rep;*/

        //TODO: compute matrix (probably going to use glm)

        transform.m_pimpl->dirty_matrix = false;
    }

    const Matrix4 &Transform3D::as_matrix(void) {
        _compute_matrix(*this);

        return m_pimpl->matrix_rep;
    }

    void Transform3D::copy_matrix(Matrix4 &target) {
        _compute_matrix(*this);

        target = m_pimpl->matrix_rep;
    }
}
