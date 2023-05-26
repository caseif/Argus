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

#include "argus/lowlevel/logging.hpp"
#include "argus/lowlevel/macros.hpp"

#include "argus/resman.hpp"

#include "argus/scripting/script_handle.hpp"
#include "internal/scripting/angelscript_loader.hpp"
#include "internal/scripting/angelscript_proxy.hpp"
#include "internal/scripting/defines.hpp"
#include "internal/scripting/module_scripting.hpp"
#include "internal/scripting/pimpl/script_handle.hpp"

#include <istream>

#include <cstddef>

namespace argus {
    AngelscriptLoader::AngelscriptLoader(void):
        ResourceLoader({ RESOURCE_TYPE_ANGELSCRIPT }) {
    }

    void *AngelscriptLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) const {
        UNUSED(manager);
        UNUSED(size);

        std::string script_src(std::istreambuf_iterator<char>(stream), {});

        const char *mod_name = proto.uid.c_str();
        const char *section_name = proto.uid.c_str();

        auto *mod = g_as_script_engine->GetModule(mod_name, asGM_ALWAYS_CREATE);

        mod->AddScriptSection(section_name, script_src.c_str());

        auto rc = mod->Build();
        if (rc < 0) {
            mod->Discard();
            throw LoadFailedException(proto.uid, "Failed to build script");
        }

        ScriptHandle *handle = new ScriptHandle();
        handle->pimpl->mod = mod;

        return handle;
    }

    [[noreturn]] void *AngelscriptLoader::copy(ResourceManager &manager, const ResourcePrototype &proto,
            void *src, std::type_index type) const {
        UNUSED(manager);
        UNUSED(proto);
        UNUSED(src);
        UNUSED(type);
        Logger::default_logger().fatal("Copy not supported for script resources");
    }

    void AngelscriptLoader::unload(void *data_ptr) const {
        auto *handle = static_cast<ScriptHandle*>(data_ptr);

        handle->pimpl->mod->Discard();

        delete handle;
    }
}
