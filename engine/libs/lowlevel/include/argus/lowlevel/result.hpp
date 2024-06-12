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

#include "argus/lowlevel/crash.hpp"
#include "argus/lowlevel/extra_type_traits.hpp"

#include <variant>

namespace argus {
    template<typename T, typename E>
    class Result;

    template<typename T, typename E>
    std::enable_if_t<!std::is_void_v<T>, Result<T, E>> ok(T value);

    template<typename T, typename E>
    std::enable_if_t<std::is_void_v<T>, Result<void, E>> ok(void);

    template<typename T, typename E>
    std::enable_if_t<!std::is_void_v<E>, Result<T, E>> err(E error);

    template<typename T, typename E>
    std::enable_if_t<std::is_void_v<E>, Result<T, void>> err(void);

    template<typename T, typename E>
    struct ResultStorage {
        #if defined(_MSC_VER) && _MSC_VER < 1930
        // workaround for VS 2019 and earlier which requires std::future's type
        // parameter type to be default-constructible
        std::optional<std::variant<reference_wrapped_t<T>, reference_wrapped_t<E>>> val_or_err;
        #else
        std::variant<reference_wrapped_t<T>, reference_wrapped_t<E>> val_or_err;
        #endif
    };

    template<typename T>
    struct ResultStorage<T, void> {
        std::optional<reference_wrapped_t<T>> val;
    };

    template<typename E>
    struct ResultStorage<void, E> {
        std::optional<reference_wrapped_t<E>> err;
    };

    template<>
    struct ResultStorage<void, void> {
        bool is_ok;
    };

    template<typename T>
    static inline std::enable_if_t<std::is_reference_v<T>, std::reference_wrapper<std::remove_reference_t<T>>>
            wrap_if_reference(T rhs) {
        return reference_wrapped_t<T>(rhs);
    }

    template<typename T>
    static inline std::enable_if_t<!std::is_reference_v<T>, T> wrap_if_reference(T rhs) {
        return rhs;
    }

    template<typename T, typename E>
    class Result {
      private:
        ResultStorage<T, E> m_storage;

      public:
        Result(ResultStorage<T, E> storage):
            m_storage(std::move(storage)) {
        }

        [[nodiscard]] bool is_ok(void) const {
            if constexpr (!std::is_void_v<T> && !std::is_void_v<E>) {
                #if defined _MSC_VER && _MSC_VER < 1930
                // workaround for VS <=2019, see comment in ResultStorage
                // definition
                return m_storage.val_or_err.value().index() == 0;
                #else
                return m_storage.val_or_err.index() == 0;
                #endif
            } else if constexpr (!std::is_void_v<T>) {
                return m_storage.val.has_value();
            } else if constexpr (!std::is_void_v<E>) {
                return !m_storage.err.has_value();
            } else {
                return m_storage.is_ok;
            }
        }

        [[nodiscard]] bool is_err(void) const {
            return !is_ok();
        }

        template<typename U = T>
        [[nodiscard]] std::enable_if_t<!std::is_void_v<U>, const U &> unwrap(void) const {
            if (!is_ok()) {
                crash_ll("Attempted to call unwrap() on error-typed Result");
            }
            if constexpr (!std::is_void_v<E>) {
                #if defined(_MSC_VER) && _MSC_VER < 1930
                // workaround for VS <=2019, see comment in ResultStorage
                // definition
                return std::get<0>(m_storage.val_or_err.value());
                #else
                return std::get<0>(m_storage.val_or_err);
                #endif
            } else {
                return m_storage.val.value();
            }
        }

        template<typename U = T>
        [[nodiscard]] std::enable_if_t<!std::is_void_v<T>, const U &> unwrap_or_default(const U &def) const {
            return is_ok() ? unwrap() : def;
        }

        template<typename U = E>
        [[nodiscard]] std::enable_if_t<!std::is_void_v<U>, const U &> unwrap_err(void) const {
            if (!is_err()) {
                crash_ll("Attempted to call unwrap() on value-typed Result");
            }
            if constexpr (!std::is_void_v<T>) {
                #if defined(_MSC_VER) && _MSC_VER < 1930
                // workaround for VS <=2019, see comment in ResultStorage
                // definition
                return std::get<1>(m_storage.val_or_err.value());
                #else
                return std::get<1>(m_storage.val_or_err);
                #endif
            } else {
                return m_storage.err.value();
            }
        }

        [[nodiscard]] Result<T, E> collate(Result<T, E> &&other) const {
            return is_ok() ? other : *this;
        }

        template<typename U, typename F>
        [[nodiscard]] Result<U, E> and_then(F op) const {
            if (is_ok()) {
                if constexpr (!std::is_void_v<T>) {
                    return ok<U, E>(op(unwrap()));
                } else {
                    return ok<U, E>(op());
                }
            } else {
                if constexpr (!std::is_void_v<E>) {
                    return err<U, E>(unwrap_err());
                } else {
                    return err<U, E>();
                }
            }
        }

        [[nodiscard]] Result<T, E> otherwise(Result<T, E> &&other) const {
            return is_ok() ? *this : other;
        }

        template<typename F, typename O>
        [[nodiscard]] Result<T, F> or_else(O op) const {
            if (is_ok()) {
                if constexpr (!std::is_void_v<T>) {
                    return ok<T, F>(unwrap());
                } else {
                    return ok<T, F>();
                }
            } else {
                if constexpr (!std::is_void_v<E>) {
                    return op(unwrap_err());
                } else {
                    return op();
                }
            }
        }

        template<typename U, typename F>
        [[nodiscard]] std::enable_if_t<!std::is_void_v<T>, Result<U, E>> map(F map_fn) const {
            if (is_ok()) {
                return ok<U, E>(map_fn(unwrap()));
            } else {
                if constexpr (!std::is_void_v<E>) {
                    return err<U, E>(unwrap_err());
                } else {
                    return err<U, E>();
                }
            }
        }

        template<typename U, typename F>
        [[nodiscard]] std::enable_if_t<!std::is_void_v<T>, U> map_or(U def, F map_fn) const {
            return is_ok() ? map_fn(unwrap()) : def;
        }

        template<typename U, typename V, typename D, typename F>
        [[nodiscard]] std::enable_if_t<!std::is_void_v<T> && !std::is_void_v<E>, U> map_or_else(
                D map_err_fn, F map_fn) const {
            return is_ok() ? map_fn(unwrap()) : map_err_fn(unwrap_err());
        }

        template<typename F, typename O>
        [[nodiscard]] std::enable_if_t<!std::is_void_v<E>, Result<T, F>> map_err(O op) const {
            if (is_err()) {
                return err<T, F>(op(unwrap_err()));
            } else {
                if constexpr (!std::is_void_v<T>) {
                    return ok<T, F>(unwrap());
                } else {
                    return ok<T, F>();
                }
            }
        }

        template<typename U = T>
        std::enable_if_t<!std::is_void_v<U>, const U &> expect(const std::string &msg) const {
            if (!is_ok()) {
                crash_ll(msg);
            }
            return unwrap();
        }

        template<typename U = T>
        std::enable_if_t<std::is_void_v<U>, void> expect(const std::string &msg) const {
            if (!is_ok()) {
                crash_ll(msg);
            }
        }

        template<typename U = E>
        std::enable_if_t<!std::is_void_v<U>, const U &> expect_err(const std::string &msg) const {
            if (!is_err()) {
                crash_ll(msg);
            }
            return unwrap_err();
        }

        template<typename U = E>
        std::enable_if_t<std::is_void_v<U>, void> expect_err(const std::string &msg) const {
            if (!is_err()) {
                crash_ll(msg);
            }
        }
    };

    template<typename T, typename E>
    std::enable_if_t<!std::is_void_v<T>, Result<T, E>> ok(T value) {
        static_assert(!std::is_void_v<T>);

        if constexpr (!std::is_void_v<E>) {
            #if defined(_MSC_VER) && _MSC_VER < 1930
            // workaround for VS <=2019, see comment in ResultStorage definition
            return Result<T, E>(ResultStorage<T, E> { std::make_optional(
                    std::variant<reference_wrapped_t<T>, reference_wrapped_t<E>> {
                            std::in_place_index<0>, wrap_if_reference<T>(value)
                    }
            ) });
            #else
            return Result<T, E>(ResultStorage<T, E> {
                    std::variant<reference_wrapped_t<T>, reference_wrapped_t<E>> {
                            std::in_place_index<0>, wrap_if_reference<T>(value)
                    }
            });
            #endif
        } else if constexpr (std::is_reference_v<T>) {
            return Result<T, E>(ResultStorage<T, E> {
                    std::make_optional(wrap_if_reference<T>(value)) });
        } else {
            return Result<T, E>(ResultStorage<T, E> {
                    std::make_optional(wrap_if_reference<T>(value)) });
        }
    }

    template<typename T, typename E>
    std::enable_if_t<std::is_void_v<T>, Result<void, E>> ok(void) {
        if constexpr (!std::is_void_v<E>) {
            return Result<void, E>(ResultStorage<void, E> { std::nullopt });
        } else {
            return Result<void, E>(ResultStorage<void, E> { true });
        }
    }

    template<typename T, typename E>
    std::enable_if_t<!std::is_void_v<E>, Result<T, E>> err(E error) {
        static_assert(!std::is_void_v<E>);

        if constexpr (!std::is_void_v<T>) {
            return Result<T, E>(ResultStorage<T, E> { std::variant<reference_wrapped_t<T>, reference_wrapped_t<E>> {
                    std::in_place_index<1>, wrap_if_reference<E>(error) } });
        } else if constexpr (std::is_reference_v<E>) {
            return Result<T, E>(ResultStorage<T, E> {
                    std::make_optional(wrap_if_reference<E>(error)) });
        } else {
            return Result<T, E>(ResultStorage<T, E> {
                    std::make_optional(wrap_if_reference<E>(error)) });
        }
    }

    template<typename T, typename E>
    std::enable_if_t<std::is_void_v<E>, Result<T, void>> err(void) {
        if constexpr (!std::is_void_v<T>) {
            return Result<T, void>(ResultStorage<T, void> { std::nullopt });
        } else {
            return Result<T, void>(ResultStorage<T, void> { false });
        }
    }
}
