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

#pragma once

#include "argus/lowlevel/macros.hpp"

#include <mutex>
#include <type_traits>

namespace argus {
    template<typename ValueType>
    struct ValueAndDirtyFlag {
        ValueType value;
        bool dirty;

        inline operator ValueType(void) const {
            return value;
        }

        template<typename V = ValueType>
        inline operator typename std::enable_if<!std::is_integral<ValueType>::value, const V &>::type(void) const {
            return value;
        }

        template<typename V = ValueType>
        typename std::enable_if<std::is_pointer<ValueType>::value, V>::type operator->(void) {
            return value;
        }

        template<typename V = ValueType>
        typename std::enable_if<!std::is_pointer<ValueType>::value, V>::type *operator->(void) {
            return &value;
        }
    };

    template<typename ValueType>
    class Dirtiable {
      private:
        ValueType value;
        mutable bool dirty;

      public:
        Dirtiable(void) :
            value(),
            dirty(false) {
        }

        Dirtiable(const ValueType &rhs) :
            value(rhs),
            dirty(false) {
        }

        Dirtiable(ValueType &&rhs) :
            value(std::move(rhs)),
            dirty(false) {
        }

        Dirtiable(const Dirtiable<ValueType> &rhs) :
            value(rhs.m_value),
            dirty(rhs.m_dirty) {
        }

        Dirtiable(Dirtiable<ValueType> &&rhs) noexcept:
            value(std::move(rhs.value)),
            dirty(rhs.dirty) {
        }

        /**
         * \brief Fetches the current value and clears the dirty flag,
                  returning both the value and the previous dirty state.
         *
         * \return A `struct` containing the copied value and the previous
         *         state of the dirty flag.
         */
        ValueAndDirtyFlag<ValueType> read(void) {
            bool old_dirty = dirty;

            dirty = false;

            return ValueAndDirtyFlag<ValueType> { value, old_dirty };
        }

        ValueAndDirtyFlag<const ValueType> read(void) const {
            bool old_dirty = dirty;

            dirty = false;

            return ValueAndDirtyFlag<const ValueType> { value, old_dirty };
        }

        /**
         * \brief Fetches the current value without affecting the dirty
         *        flag.
         *
         * \return A copy of the current value.
         */
        const ValueType &peek(void) const {
            return value;
        }

        /**
         * \brief Performs a copy assignment to an lvalue, carrying over the
         *        current value and dirty flag.
         *
         * \param rhs The Dirtiable to assign.
         *
         * \return This Dirtiable.
         */
        inline Dirtiable &operator=(const Dirtiable<ValueType> &rhs) {
            this->value = rhs.value;
            this->dirty = rhs.dirty;
            return *this;
        }

        /**
         * \brief Performs an assignment to an lvalue, setting the dirty
         *        flag.
         *
         * \param rhs The value to assign.
         *
         * \return This Dirtiable.
         */
        inline Dirtiable &operator=(const ValueType &rhs) {
            value = rhs;
            dirty = true;
            return *this;
        }

        /**
         * \brief Performs an assignment to an rvalue, setting the dirty
         *        flag.
         *
         * \param rhs The value to assign.
         *
         * \return This Dirtiable.
         */
        inline Dirtiable &operator=(const ValueType &&rhs) {
            value = std::move(rhs);
            dirty = true;
            return *this;
        }

        inline Dirtiable &operator+=(const ValueType &rhs) {
            value += rhs;
            dirty = true;
            return *this;
        }

        inline Dirtiable &operator-=(const ValueType &rhs) {
            value -= rhs;
            dirty = true;
            return *this;
        }

        inline Dirtiable &operator*=(const ValueType &rhs) {
            value *= rhs;
            dirty = true;
            return *this;
        }

        inline Dirtiable &operator/=(const ValueType &rhs) {
            value /= rhs;
            dirty = true;
            return *this;
        }

        /**
         * Performs an assignment to an lvalue without setting the dirty
         * flag.
         *
         * \param rhs The value to assign.
         */
        void set_quietly(const ValueType &rhs) {
            value = rhs;
        }

        /**
         * Performs an assignment to an rvalue without setting the dirty
         * flag.
         *
         * \param rhs The value to assign.
         */
        void set_quietly(const ValueType &&rhs) {
            value = std::move(rhs);
        }
    };
}
