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

#include "argus/lowlevel/result.hpp"

#include "argus/resman/resource.hpp"

#include "argus/scripting/script_context.hpp"

#include <string>
#include <vector>

namespace argus {
    class ScriptingLanguagePlugin {
      public:
        ScriptingLanguagePlugin(void) = default;

        ScriptingLanguagePlugin(const ScriptingLanguagePlugin &) = delete;

        ScriptingLanguagePlugin(ScriptingLanguagePlugin &&) = delete;

        virtual ~ScriptingLanguagePlugin(void) = default;

        virtual const char *get_language_name(void) const = 0;

        virtual const std::vector<std::string> &get_media_types(void) const = 0;

        virtual void *create_context_data(void) = 0;

        virtual void destroy_context_data(void *data) = 0;

        virtual Result<void, ScriptLoadError> load_script(ScriptContext &context, const Resource &resource) = 0;

        virtual void bind_type(ScriptContext &context, const BoundTypeDef &type) = 0;

        virtual void bind_type_function(ScriptContext &context, const BoundTypeDef &type,
                const BoundFunctionDef &fn) = 0;

        virtual void bind_type_field(ScriptContext &context, const BoundTypeDef &type,
                const BoundFieldDef &field) = 0;

        virtual void bind_global_function(ScriptContext &context, const BoundFunctionDef &fn) = 0;

        virtual void bind_enum(ScriptContext &context, const BoundEnumDef &enum_def) = 0;

        virtual Result<ObjectWrapper, ScriptInvocationError> invoke_script_function(ScriptContext &context,
                const std::string &name, const std::vector<ObjectWrapper *> &params) = 0;
    };
}
