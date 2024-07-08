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
#include "internal/render/pimpl/common/transform_2d.hpp"

#include <cmath>

namespace argus {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_Transform2D));

    static void _inc_version(Transform2D &transform) {
        if (transform.m_pimpl->version_ptr != nullptr) {
            (*transform.m_pimpl->version_ptr)++;
        }
    }

    Transform2D::Transform2D(void):
        Transform2D({ 0, 0 }, 0, { 1, 1 }) {
    }

    Transform2D::Transform2D(const Vector2f &translation, float rotation, const Vector2f &scale):
        m_pimpl(&g_pimpl_pool.construct<pimpl_Transform2D>(translation, rotation, scale)) {
    }

    Transform2D::Transform2D(const Transform2D &rhs) noexcept:
        Transform2D(rhs.m_pimpl->translation, rhs.m_pimpl->rotation, rhs.m_pimpl->scale) {
    }

    // for the move ctor, we just steal the m_pimpl
    Transform2D::Transform2D(Transform2D &&rhs) noexcept:
        m_pimpl(rhs.m_pimpl) {
        rhs.m_pimpl = reinterpret_cast<pimpl_Transform2D *>(0xDEADBEEF);
    }

    Transform2D::~Transform2D(void) {
        if (m_pimpl != nullptr && m_pimpl != reinterpret_cast<pimpl_Transform2D *>(0xDEADBEEF)) {
            g_pimpl_pool.destroy(m_pimpl);
        }
    }

    Transform2D &Transform2D::operator=(const Transform2D &rhs) noexcept {
        m_pimpl->translation = rhs.m_pimpl->translation;
        m_pimpl->rotation.store(rhs.m_pimpl->rotation);
        m_pimpl->scale = rhs.m_pimpl->scale;
        m_pimpl->dirty_matrix = &rhs != this || m_pimpl->dirty_matrix;

        _inc_version(*this);

        return *this;
    }

    Transform2D &Transform2D::operator=(Transform2D &&rhs) noexcept {
        m_pimpl->translation = rhs.m_pimpl->translation;
        m_pimpl->rotation.store(std::move(rhs.m_pimpl->rotation));
        m_pimpl->scale = rhs.m_pimpl->scale;
        m_pimpl->dirty_matrix = &rhs != this || std::move(m_pimpl->dirty_matrix);

        _inc_version(*this);

        return *this;
    }

    Transform2D Transform2D::operator+(const Transform2D &rhs) const {
        return Transform2D(
                m_pimpl->translation + rhs.m_pimpl->translation,
                m_pimpl->rotation.load() + rhs.m_pimpl->rotation,
                m_pimpl->scale * rhs.m_pimpl->scale
        );
    }

    Vector2f Transform2D::get_translation(void) const {
        m_pimpl->translation_mutex.lock();
        Vector2f translation_current = m_pimpl->translation;
        m_pimpl->translation_mutex.unlock();

        return translation_current;
    }

    void Transform2D::set_translation(const Vector2f &translation) {
        m_pimpl->translation_mutex.lock();
        m_pimpl->translation = translation;
        m_pimpl->translation_mutex.unlock();

        m_pimpl->set_dirty();
        _inc_version(*this);
    }

    void Transform2D::set_translation(float x, float y) {
        this->set_translation({ x, y });
    }

    void Transform2D::add_translation(const Vector2f &translation_delta) {
        m_pimpl->translation_mutex.lock();
        m_pimpl->translation += translation_delta;
        m_pimpl->translation_mutex.unlock();

        m_pimpl->set_dirty();
        _inc_version(*this);
    }

    void Transform2D::add_translation(float x, float y) {
        this->add_translation({ x, y });
    }

    float Transform2D::get_rotation(void) const {
        return m_pimpl->rotation;
    }

    void Transform2D::set_rotation(float rotation_radians) {
        m_pimpl->rotation = rotation_radians;

        m_pimpl->set_dirty();
        _inc_version(*this);
    }

    void Transform2D::add_rotation(float rotation_radians) {
        float current = m_pimpl->rotation.load();
        float updated = float(fmod(current + rotation_radians, 2 * M_PI));
        while (!m_pimpl->rotation.compare_exchange_weak(current, updated));
        m_pimpl->set_dirty();
        _inc_version(*this);
    }

    Vector2f Transform2D::get_scale(void) const {
        m_pimpl->scale_mutex.lock();
        Vector2f scale_current = m_pimpl->scale;
        m_pimpl->scale_mutex.unlock();

        return scale_current;
    }

    void Transform2D::set_scale(const Vector2f &scale) {
        m_pimpl->scale_mutex.lock();
        m_pimpl->scale = scale;
        m_pimpl->scale_mutex.unlock();

        m_pimpl->set_dirty();
        _inc_version(*this);
    }

    void Transform2D::set_scale(float x, float y) {
        this->set_scale({ x, y });
    }

    static void _compute_aux_matrices(const Transform2D &transform) {
        if (!transform.m_pimpl->dirty_matrix) {
            return;
        }

        float cos_rot = std::cos(transform.m_pimpl->rotation);
        float sin_rot = std::sin(transform.m_pimpl->rotation);

        transform.m_pimpl->translation_mutex.lock();
        Vector2f translation_current = transform.m_pimpl->translation;
        transform.m_pimpl->translation_mutex.unlock();

        transform.m_pimpl->scale_mutex.lock();
        Vector2f scale_current = transform.m_pimpl->scale;
        transform.m_pimpl->scale_mutex.unlock();

        transform.m_pimpl->translation_matrix = Matrix4::from_row_major({
                1, 0, 0, translation_current.x,
                0, 1, 0, translation_current.y,
                0, 0, 1, 0,
                0, 0, 0, 1
        });

        transform.m_pimpl->rotation_matrix = Matrix4::from_row_major({
                cos_rot, -sin_rot, 0, 0,
                sin_rot, cos_rot, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
        });

        transform.m_pimpl->scale_matrix = Matrix4::from_row_major({
                scale_current.x, 0, 0, 0,
                0, scale_current.y, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
        });

        transform.m_pimpl->dirty_matrix = false;
    }

    static void _compute_transform_matrix(const Transform2D &transform, const Vector2f &anchor_point) {
        //auto cur_translation = transform.get_translation();

        //UNUSED(anchor_point);
        Matrix4 anchor_mat_1 = Matrix4::from_row_major({
                1, 0, 0, -anchor_point.x,
                0, 1, 0, -anchor_point.y,
                0, 0, 1, 0,
                0, 0, 0, 1
        });
        Matrix4 anchor_mat_2 = Matrix4::from_row_major({
                1, 0, 0, anchor_point.x,
                0, 1, 0, anchor_point.y,
                0, 0, 1, 0,
                0, 0, 0, 1
        });
        UNUSED(anchor_mat_1);
        UNUSED(anchor_mat_2);
        transform.m_pimpl->matrix_rep
                = transform.get_translation_matrix()
                * anchor_mat_2
                * transform.get_rotation_matrix()
                * transform.get_scale_matrix()
                * anchor_mat_1;
    }

    static void _compute_matrices(const Transform2D &transform, const Vector2f &anchor_point) {
        bool dirty = transform.m_pimpl->dirty_matrix;

        if (dirty) {
            _compute_aux_matrices(transform);
        }

        if (dirty || anchor_point != transform.m_pimpl->last_anchor_point) {
            _compute_transform_matrix(transform, anchor_point);
        }

        transform.m_pimpl->dirty_matrix = false;
    }

    const Matrix4 &Transform2D::as_matrix(const Vector2f &anchor_point) const {
        _compute_matrices(*this, anchor_point);

        return m_pimpl->matrix_rep;
    }

    const Matrix4 &Transform2D::get_translation_matrix(void) const {
        _compute_aux_matrices(*this);

        return m_pimpl->translation_matrix;
    }

    const Matrix4 &Transform2D::get_rotation_matrix(void) const {
        _compute_aux_matrices(*this);

        return m_pimpl->rotation_matrix;
    }

    const Matrix4 &Transform2D::get_scale_matrix(void) const {
        _compute_aux_matrices(*this);

        return m_pimpl->scale_matrix;
    }

    void Transform2D::copy_matrix(Matrix4 &target, const Vector2f &anchor_point) const {
        _compute_matrices(*this, anchor_point);

        target = m_pimpl->matrix_rep;
    }

    Transform2D Transform2D::inverse(void) const {
        return { this->m_pimpl->translation.inverse(), -this->m_pimpl->rotation, this->m_pimpl->scale };
    }

    // this allows the transform to automatically increment the version of an
    // enclosing object
    void Transform2D::set_version_ref(uint16_t &version_ref) {
        this->m_pimpl->version_ptr = &version_ref;
    }
}
