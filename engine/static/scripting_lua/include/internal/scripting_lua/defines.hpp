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

namespace argus {
    constexpr const char *k_resource_type_lua = "text/x-lua";

    constexpr const char *k_reg_key_plugin_ptr = "argus_plugin";
    constexpr const char *k_reg_key_context_data_ptr = "argus_context_data";

    constexpr const char *k_plugin_lang_name = "lua";

    constexpr const char *k_lua_index = "__index";
    constexpr const char *k_lua_newindex = "__newindex";
    constexpr const char *k_lua_name = "__name";
    constexpr const char *k_lua_require = "require";
    constexpr const char *k_lua_require_def = "default_require";

    constexpr const char *k_clone_fn = "clone";

    constexpr const char *k_mt_vector = "_internal_vector";
    constexpr const char *k_mt_vector_ref = "_internal_vectorref";

    constexpr const char *k_const_prefix = "const ";

    constexpr const char *k_engine_namespace = "argus";

    constexpr const char *k_empty_repl = "(empty)";
}
