/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

// module lowlevel
#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/common/transform.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"

#include <atomic>
#include <mutex>

#include <cmath>
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
            g_pimpl_pool.destroy(pimpl);
        }
    }

    Transform2D &Transform2D::operator=(const Transform2D &rhs) noexcept {
        pimpl->translation = rhs.pimpl->translation;
        pimpl->rotation.store(rhs.pimpl->rotation);
        pimpl->scale = rhs.pimpl->scale;
        pimpl->dirty = &rhs != this || pimpl->dirty;
        pimpl->dirty_matrix = &rhs != this || pimpl->dirty_matrix;

        return *this;
    }

    Transform2D Transform2D::operator +(const Transform2D &rhs) const {
        return Transform2D(
                pimpl->translation + rhs.pimpl->translation,
                pimpl->rotation.load() + rhs.pimpl->rotation,
                pimpl->scale * rhs.pimpl->scale
        );
    }

    Vector2f Transform2D::get_translation(void) const {
        pimpl->translation_mutex.lock();
        Vector2f translation_current = pimpl->translation;
        pimpl->translation_mutex.unlock();

        return translation_current;
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

    float Transform2D::get_rotation(void) const {
        return pimpl->rotation;
    }

    void Transform2D::set_rotation(const float rotation_radians) {
        pimpl->rotation = rotation_radians;

        pimpl->set_dirty();
    }

    void Transform2D::add_rotation(const float rotation_radians) {
        float current = pimpl->rotation.load();
        float updated = float(fmod(current + rotation_radians, 2 * M_PI));
        while (!pimpl->rotation.compare_exchange_weak(current, updated));
        pimpl->set_dirty();
    }

    Vector2f Transform2D::get_scale(void) const {
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

        transform.pimpl->matrix_rep = {
            {cos_rot * scale_current.x, -sin_rot, 0, translation_current.x},
            {sin_rot, cos_rot * scale_current.y, 0, translation_current.y},
            {0, 0, 1, 0},
            {0, 0, 0, 1}
        };

        transform.pimpl->dirty_matrix = false;
    }

    const Matrix4 &Transform2D::as_matrix(void) {
        _compute_matrix(*this);

        return pimpl->matrix_rep;
    }

    void Transform2D::copy_matrix(Matrix4 &target) {
        _compute_matrix(*this);

        target = pimpl->matrix_rep;
    }

}
