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

#include "argus/scripting/types.hpp"

#include <string>
#include <thread>
#include <vector>

namespace argus {
    // forward declarations
    class Resource;

    struct pimpl_ScriptContext;

    class ScriptContext {
      public:
        pimpl_ScriptContext *m_pimpl;

        ScriptContext(std::string language, void *data);

        ScriptContext(ScriptContext &) = delete;

        ScriptContext(ScriptContext &&) = delete;

        ~ScriptContext(void);

        void load_script(const std::string &uid);

        void load_script(const Resource &resource);

        ObjectWrapper invoke_script_function(const std::string &fn_name, const std::vector<ObjectWrapper> &params);

        void *get_plugin_data_ptr(void);

        template<typename T>
        T *get_plugin_data(void) {
            return reinterpret_cast<T *>(get_plugin_data_ptr());
        }
    };

    ScriptContext &create_script_context(const std::string &language);

    void destroy_script_context(ScriptContext &context);

    ScriptContext &load_script(const std::string &uid);
}
