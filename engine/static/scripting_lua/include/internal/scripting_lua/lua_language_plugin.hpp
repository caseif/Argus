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

#include "argus/scripting/script_context.hpp"
#include "argus/scripting/scripting_language_plugin.hpp"

namespace argus {
    class LuaLanguagePlugin : public ScriptingLanguagePlugin {
      public:
        LuaLanguagePlugin(void);

        LuaLanguagePlugin(LuaLanguagePlugin &) = delete;

        LuaLanguagePlugin(LuaLanguagePlugin &&) = delete;

        ~LuaLanguagePlugin() override;

        const char *get_language_name(void) const override;

        const std::vector<std::string> &get_media_types(void) const override;

        void *create_context_data(void) override;

        void destroy_context_data(void *data) override;

        [[nodiscard]] Result<void, ScriptLoadError> load_script(ScriptContext &context,
                const Resource &resource) override;

        void bind_type(ScriptContext &context, const BoundTypeDef &type) override;

        void bind_global_function(ScriptContext &context, const BoundFunctionDef &fn) override;

        void bind_enum(ScriptContext &context, const BoundEnumDef &enum_def) override;

        [[nodiscard]] Result<ObjectWrapper, ScriptInvocationError> invoke_script_function(ScriptContext &context,
                const std::string &name, const std::vector<ObjectWrapper *> &params) override;
    };
}
