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

#pragma once

#include "argus/lowlevel/dirtiable.hpp"
#include "argus/lowlevel/macros.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h> // need it for rwlock
#endif

namespace argus {
    /**
     * \brief A drop-in replacement for std::atomic for non-trvially copyable
     *        types.
     *
     * Because std::atomic generally operates on primitive types only, it cannot
     * be used with complex types such as std::string. A ComplexAtomic object
     * wraps an object not eligible for use with std::atomic and provides
     * transparent atomicity support in a similar fashion.
     *
     * \tparam ValueType The type of value to be wrapped for atomic use.
     */
    template <typename ValueType>
    class ComplexAtomic {
        private:
            ValueType value;
            std::mutex mutex;

        public:
            /**
             * \brief The default constructor; creates a `ComplexAtomic` with an
             *        empty value.
             */
            ComplexAtomic(void):
                    value() {
            }

            /**
             * \brief The copy constructor.
             *
             * \param val The value to copy into this `ComplexAtomic`'s state.
             */
            explicit ComplexAtomic(ValueType &val):
                    value(val) {
            }

            /**
             * \brief The move constructor.
             *
             * \param val The value to move into this `ComplexAtomic`'s state.
             */
            explicit ComplexAtomic(ValueType &&val):
                    value(val) {
            }

            /**
             * \brief Converts the ComplexAtomic to its base type, effectively
             * "unwrapping" it.
             *
             * \return The base value.
             */
            inline operator ValueType(void) {
                mutex.lock();
                ValueType val_copy = value;
                mutex.unlock();
                return val_copy;
            }

            /**
             * \brief Performs an atomic assignment to an lvalue.
             *
             * \param rhs The value to assign.
             *
             * \return This ComplexAtomic.
             */
            inline ComplexAtomic &operator =(const ValueType &rhs) {
                mutex.lock();
                value = rhs;
                mutex.unlock();
                return *this;
            }

            /**
             * \brief Performs an atomic assignment to an rvalue.
             *
             * \param rhs The value to assign.
             *
             * \return This ComplexAtomic.
             */
            inline ComplexAtomic &operator =(const ValueType &&rhs) {
                mutex.lock();
                value = std::move(rhs);
                mutex.unlock();
                return *this;
            }

            ValueType &operator *(void) = delete;
    };

    /**
     * \brief Represents a value which is to be read and written atomically,
     *        and contains a "dirtiness" attribute.
     *
     * An `AtomicDirtiable` is essentially equivalent to a ComplexAtomic, but
     * contains an additional std::atomic_bool attribute to track its dirtiness.
     *
     * \tparam ValueType The type of value to be wrapped for use.
     */
    template <typename ValueType>
    class AtomicDirtiable {
        private:
            ValueType value;
            bool dirty;
            std::mutex mutex;

            template <typename V = ValueType>
            typename std::enable_if<std::is_integral<V>::value, bool>::type
            is_same_value(V &new_val) {
                return new_val == this->value;
            }

            template <typename V = ValueType>
            typename std::enable_if<!std::is_integral<V>::value, bool>::type
            is_same_value(V &new_val) {
                UNUSED(new_val);
                return false;
            }

        public:
            AtomicDirtiable(void):
                value(),
                dirty(false),
                mutex() {
            }

            AtomicDirtiable(const ValueType &rhs):
                value(rhs),
                dirty(false),
                mutex() {
            }

            AtomicDirtiable(ValueType &&rhs):
                value(std::move(rhs)),
                dirty(false),
                mutex() {
            }

            AtomicDirtiable(const AtomicDirtiable &rhs):
                value(rhs.value),
                dirty(rhs.dirty),
                mutex() {
            }

            AtomicDirtiable(AtomicDirtiable &&rhs):
                value(std::move(rhs.value)),
                dirty(rhs.dirty),
                mutex() {
            }

            /**
             * \brief Atomically fetches the current value and clears the dirty
             *        flag, returning both the copied value and the previous
             *        dirty state.
             *
             * \return A `struct` containing the copied value and the previous
             *         state of the dirty flag.
             */
            const ValueAndDirtyFlag<ValueType> read(void) {
                mutex.lock();

                auto val_copy = value;
                bool old_dirty = dirty;
                
                dirty = false;

                mutex.unlock();

                return ValueAndDirtyFlag<ValueType> { val_copy, old_dirty };
            };

            /**
             * \brief Atomically fetches the current value without affecting the
                      dirty flag.
             *
             * \return A copy of the current value.
             */
            ValueType peek(void) {
                mutex.lock();
                auto val_copy = value;
                mutex.unlock();

                return val_copy;
            };

            /**
             * \brief Performs an atomic assignment to an lvalue, setting the
             *        dirty flag.
             *
             * \param rhs The value to assign.
             *
             * \return This AtomicDirtiable.
             */
            inline AtomicDirtiable &operator =(const ValueType &rhs) {
                mutex.lock();
                if (!is_same_value(rhs)) {
                    value = rhs;
                    dirty = true;
                }
                mutex.unlock();
                return *this;
            };

            /**
             * \brief Performs an atomic assignment to an rvalue, setting the
             *        dirty flag.
             *
             * \param rhs The value to assign.
             *
             * \return This AtomicDirtiable.
             */
            inline AtomicDirtiable &operator =(const ValueType &&rhs) {
                mutex.lock();
                if (!is_same_value(rhs)) {
                    value = std::move(rhs);
                    dirty = true;
                }
                mutex.unlock();
                return *this;
            };

            /**
             * Performs an atomic assignment to an lvalue without setting the
             * dirty flag.
             *
             * \param rhs The value to assign.
             */
            void set_quietly(const ValueType &rhs) {
                mutex.lock();
                value = rhs;
                mutex.unlock();
            }

            /**
             * Performs an atomic assignment to an rvalue without setting the
             * dirty flag.
             *
             * \param rhs The value to assign.
             */
            void set_quietly(const ValueType &&rhs) {
                mutex.lock();
                value = std::move(rhs);
                mutex.unlock();
            }
    };
}
