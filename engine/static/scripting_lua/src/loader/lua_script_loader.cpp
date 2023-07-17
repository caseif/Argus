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

#include "argus/lowlevel/macros.hpp"

#include "argus/resman.hpp"

#include "internal/scripting_lua/defines.hpp"
#include "internal/scripting_lua/loaded_script.hpp"
#include "internal/scripting_lua/loader/lua_script_loader.hpp"

namespace argus {
    LuaScriptLoader::LuaScriptLoader(void) : ResourceLoader({ RESOURCE_TYPE_LUA }) {
    }

    LuaScriptLoader::~LuaScriptLoader(void) = default;

    void *LuaScriptLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) const {
        UNUSED(manager);
        UNUSED(proto);
        UNUSED(size);

        std::string script_src(std::istreambuf_iterator<char>(stream), {});

        return new LoadedScript(script_src);
    }

    void *LuaScriptLoader::copy(ResourceManager &manager, const ResourcePrototype &proto,
            void *src, std::type_index type) const {
        UNUSED(manager);
        UNUSED(proto);
        UNUSED(type);

        auto *loaded_script = reinterpret_cast<LoadedScript *>(src);
        return new LoadedScript(loaded_script->source);
    }

    void LuaScriptLoader::unload(void *data_ptr) const {
        delete reinterpret_cast<LoadedScript *>(data_ptr);
    }
}
