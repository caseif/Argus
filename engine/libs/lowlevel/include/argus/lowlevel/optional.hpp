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

#pragma once

#include <exception>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace argus {
    /**
     * \brief Limited implementation of std::optional from C++17.
     *
     * This class does not implement all features of std::optional in the C++17
     * specification, and while some effort has been made to adhere as closely
     * to the specification as is practical, conformance is not guaranteed.
     */
    template <typename T>
    class Optional {
        private:
            bool present;
            T val;

            constexpr void throw_if_not_present(void) const {
                if (!present) {
                    throw std::runtime_error("value() called on non-present Optional");
                }
            }
        public:
            Optional<T>(void) noexcept: present(false) {
            }

            Optional<T>(std::nullptr_t) noexcept: present(false) {
            }

            template <typename U = T,
                typename std::enable_if<std::is_copy_constructible<U>::value, int>::type = 0>
            Optional<T>(const Optional<T> &other):
                    present(other.present) {
                if (other.present) {
                    val(other.val);
                }
            }

            template <typename U = T,
                typename std::enable_if<std::is_move_constructible<U>::value, int>::type = 0>
            Optional<T>(Optional<T> &&other) noexcept:
                    present(other.present) {
                if (other.present) {
                    val = std::move(other.val);
                }
            }

            template <typename U = T,
                typename... Args,
                typename std::enable_if<std::is_constructible<T, std::initializer_list<U>&, Args&&...>::value, int>::type = 0>
            Optional<T>(Args &&... args) noexcept:
                    present(true),
                    val(std::forward<U>(args)...) {
            }

            template <typename U = T,
                typename std::enable_if<std::is_constructible<T, U&&>::value, int>::type = 0>
            Optional<T>(U &&val) noexcept:
                    present(true),
                    val(std::forward<U>(val)) {
            }

            ~Optional(void) {
                if (!std::is_trivially_destructible<T>() && this->present) {
                    this->val.~T();
                }
            }

            Optional<T> &operator =(std::nullptr_t) noexcept {
                present = false;
                return *this;
            }

            Optional<T> &operator =(const Optional<T> &other) {
                if (other.present) {
                    if (this->present) {
                        val = other.val;
                    } else {
                        val(other.val);
                        this->present = true;
                    }
                } else {
                    if (this->present) {
                        val.~T();
                        this->present = false;
                    } else {
                        return *this;
                    }
                }

                return *this;
            }

            Optional<T> &operator =(Optional<T> &&other) noexcept {
                if (other.present) {
                    if (this->present) {
                        val = std::move(other.val);
                    } else {
                        val(std::move(other.val));
                        this->present = true;
                    }
                } else {
                    if (this->present) {
                        val.~T();
                        this->present = false;
                    } else {
                        return *this;
                    }
                }

                return *this;
            }

            template<class U = T,
                typename std::enable_if<std::is_assignable<T&, U>::value>::type = 0>
            Optional<T> &operator =(U &&value) {
                if (this->present) {
                    this->val = std::forward(value);
                } else {
                    this->val(std::forward(value));
                }
            }

            constexpr operator T&() noexcept {
                return val;
            }

            constexpr const T *operator ->() const noexcept {
                return &val;
            }

            constexpr T *operator ->() noexcept {
                return &val;
            }

            constexpr const T &operator *() const & noexcept {
                return val;
            }

            constexpr T &operator &() & noexcept {
                return val;
            }

            constexpr const T &&operator *() const && noexcept {
                return std::move(val);
            }

            constexpr T &&operator *() && noexcept {
                return std::move(val);
            }

            constexpr explicit operator bool(void) const noexcept {
                return this->has_value();
            }

            constexpr bool has_value(void) const noexcept {
                return this->present;
            }

            constexpr T &value(void) & {
                throw_if_not_present();
                return this->val;
            }

            constexpr const T &value(void) const & {
                throw_if_not_present();
                return this->val;
            }
            constexpr T &&value(void) && {
                throw_if_not_present();
                return this->val;
            }

            constexpr const T &&value(void) const && {
                throw_if_not_present();
                return this->val;
            }

            template <typename U>
            constexpr T value_or(U &&default_value) const & {
                return this->present ? val : static_cast<T>(std::forward<U>(default_value));
            }

            template <typename U>
            constexpr T value_or(U &&default_value) && {
                return this->present ? std::move(val) : static_cast<T>(std::forward<U>(default_value));
            }

            void reset(void) noexcept {
                if (this->present) {
                    this->val.~T();
                    this->present = false;
                }
            }
    };
}
