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

#include <array>
#include <functional>
#include <optional>
#include <tuple>
#include <vector>

namespace argus {
    template <typename T>
    struct reference_wrapped {
        using type = T;
    };

    template <typename T>
    struct reference_wrapped<T &> {
        using type = std::reference_wrapper<T>;
    };

    template <typename T>
    struct reference_wrapped<T &&> {
        using type = std::reference_wrapper<T>;
    };

    template <typename T>
    using reference_wrapped_t = typename reference_wrapped<T>::type;

    template <typename T>
    struct function_traits;

    template <typename Ret, typename... Args>
    struct function_traits<Ret(*)(Args...)> {
        using class_type = void;
        using return_type = Ret;
        using argument_types = std::tuple<Args...>;
        using argument_types_wrapped = std::tuple<reference_wrapped_t<Args>...>;
        using is_const = std::false_type;
    };

    template <typename Ret, typename... Args>
    struct function_traits<Ret(*)(Args...) noexcept> {
        using class_type = void;
        using return_type = Ret;
        using argument_types = std::tuple<Args...>;
        using argument_types_wrapped = std::tuple<reference_wrapped_t<Args>...>;
        using is_const = std::false_type;
    };

    template <typename Class, typename Ret, typename... Args>
    struct function_traits<Ret(Class::*)(Args...)> {
        using class_type = Class;
        using return_type = Ret;
        using argument_types = std::tuple<Args...>;
        using argument_types_wrapped = std::tuple<reference_wrapped_t<Args>...>;
        using is_const = std::false_type;
    };

    template <typename Class, typename Ret, typename... Args>
    struct function_traits<Ret(Class::*)(Args...) const> {
        using class_type = Class;
        using return_type = Ret;
        using argument_types = std::tuple<Args...>;
        using argument_types_wrapped = std::tuple<reference_wrapped_t<Args>...>;
        using is_const = std::true_type;
    };

    template <typename Class, typename Ret, typename... Args>
    struct function_traits<Ret(Class::*)(Args...) noexcept> {
        using class_type = Class;
        using return_type = Ret;
        using argument_types = std::tuple<Args...>;
        using argument_types_wrapped = std::tuple<reference_wrapped_t<Args>...>;
        using is_const = std::true_type;
    };

    template <typename Class, typename Ret, typename... Args>
    struct function_traits<Ret(Class::*)(Args...) const noexcept> {
        using class_type = Class;
        using return_type = Ret;
        using argument_types = std::tuple<Args...>;
        using argument_types_wrapped = std::tuple<reference_wrapped_t<Args>...>;
        using is_const = std::true_type;
    };

    template <typename Ret, typename... Args>
    struct function_traits<std::function<Ret(Args...)>> {
        using class_type = void;
        using return_type = Ret;
        using argument_types = std::tuple<Args...>;
        using argument_types_wrapped = std::tuple<reference_wrapped_t<Args>...>;
        using is_const = std::false_type;
    };

    template <typename FieldType>
    struct field_traits;

    template <typename Class, typename T>
    struct field_traits<T Class::*> {
        using class_type = Class;
        using field_type = T;
        using is_const = std::false_type;
    };

    template <typename F>
    struct is_std_function : std::false_type {};

    template <typename F>
    struct is_std_function<std::function<F>> : std::true_type {};

    template <typename F>
    constexpr bool is_std_function_v = is_std_function<F>::value;

    template <typename F>
    struct is_std_vector : std::false_type {};

    template <typename F>
    struct is_std_vector<std::vector<F>> : std::true_type {};

    template <typename F>
    constexpr bool is_std_vector_v = is_std_vector<F>::value;

    template <typename T>
    struct remove_vector {
        using type = T;
    };

    template <typename T>
    struct remove_vector<std::vector<T>> {
        using type = T;
    };

    template <typename T>
    using remove_vector_t = typename remove_vector<T>::type;

    template <typename F>
    struct is_std_array : std::false_type {};

    template <typename F, size_t N>
    struct is_std_array<std::array<F, N>> : std::true_type {};

    template <typename F>
    constexpr bool is_std_array_v = is_std_array<F>::value;

    template <typename T>
    struct remove_array {
        using type = T;
    };

    template <typename T, size_t N>
    struct remove_array<std::array<T, N>> {
        using type = T;
    };

    template <typename T>
    using remove_array_t = typename remove_array<T>::type;

    template <typename T>
    struct is_reference_wrapper : std::false_type {};

    template <typename T>
    struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type {};

    template <typename T>
    constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

    template <typename T>
    struct remove_reference_wrapper {
        using type = T;
    };

    template <typename T>
    struct remove_reference_wrapper<std::reference_wrapper<T>> {
        using type = T;
    };

    template <typename T>
    using remove_reference_wrapper_t = typename remove_reference_wrapper<T>::type;

    template <typename T>
    struct remove_initializer_list {
        using type = T;
    };

    template <typename T>
    struct remove_initializer_list<std::initializer_list<T>> {
        using type = T;
    };

    template <typename T>
    using remove_initializer_list_t = typename remove_initializer_list<T>::type;

    template<class> inline constexpr bool always_false_v = false;

    template <typename T, template <typename...> typename Template>
    struct is_specialization : std::false_type {};

    template <template <typename...> typename Template, typename... Args>
    struct is_specialization<Template<Args...>, Template> : std::true_type {};

    template <typename T, template <typename...> typename Template>
    constexpr bool is_specialization_v = is_specialization<T, Template>::value;
}
