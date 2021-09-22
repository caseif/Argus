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

#include <functional>
#include <string>
#include <vector>

#include <cstddef>

#ifdef _WIN32
// required for REGISTER_ARGUS_MODULE macro
#include <windows.h>
#endif

constexpr const char *ModuleCore = "core";
constexpr const char *ModuleWm = "wm";
constexpr const char *ModuleEcs = "ecs";
constexpr const char *ModuleInput = "input";
constexpr const char *ModuleResman = "resman";
constexpr const char *ModuleRender = "render";

/**
 * \brief Macro for conveniently registering Argus modules contained by shared
 *        libraries.
 *
 * This macro implicitly invokes argus::register_module upon library load.
 *
 * \remark If desired, the library entry point may be specified manually and the
 * argus::register_argus_module function invoked explicitly, removing any need
 * for this macro.
 *
 * \param id The ID of the module.
 * \param layer The layer of the module.
 * \param dependencies A list of IDs of modules this one is dependent on.
 * \param lifecycle_update_callback The function which handles lifecycle updates
 *        for this module.
 *
 * \sa argus::ArgusModule
 * \sa argus::register_argus_module
 */
#ifdef _WIN32
#define REGISTER_ARGUS_MODULE(id, dependencies, lifecycle_update_callback) \
    BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) { \
        argus::register_module(argus::DynamicModule{id, dependencies, lifecycle_update_callback}); \
        return true; \
    }
#elif defined(__GNUC__) || defined(__clang__)
#define REGISTER_ARGUS_MODULE(id, dependencies, lifecycle_update_callback) \
    __attribute__((constructor)) static void __argus_module_ctor(void) { \
        argus::register_module(argus::DynamicModule{id, dependencies, lifecycle_update_callback}); \
    }
#else
#error This platform is not supported.
#endif

namespace argus {
    /**
     * \brief Represents the stages of engine bring-up or spin-down.
     */
    enum class LifecycleStage {
        /**
         * \brief The first standard lifecycle stage.
         *
         * Should be used for performing early allocation or other early setup,
         * generally for the purpose of preparing the module for use in the
         * initialization of dependent modules.
         */
        PreInit,
        /**
         * \brief Primary initialization stage, for performing most
         *        initialization tasks.
         */
        Init,
        /**
         * \brief Post-initialization stage, for performing initialization
         *        contingent on all parent modules being initialized.
         */
        PostInit,
        /**
         * \brief Early de-initialization. This occurs immediately after the
         *        engine has committed to shutting down.
         *
         * Should be used for performing early de-initialization tasks, such as
         * saving user data. Changes during this stage should not be visible to
         * dependent modules.
         */
        PreDeinit,
        /**
         * \brief Primary de-initialization.
         *
         * Should be used for performing most de-initialization tasks.
         */
        Deinit,
        /**
         * \brief Very late de-initialization.
         *
         * Should be used for performing de-init contingent on parent modules
         * being fully de-initialized as well as for final deallocation and
         * similar tasks.
         */
        PostDeinit
    };

    /**
     * \brief A callback for passing lifecycle changes to engine modules.
     */
    typedef std::function<void(const LifecycleStage)> LifecycleUpdateCallback;

    /**
     * \brief Represents a module to be dynamically loaded by the Argus engine.
     *
     * This struct contains all information required to initialize and update
     * the module appropriately.
     */
    struct DynamicModule {
        /**
         * \brief The ID of the module.
         *
         * \attention This ID must contain only lowercase Latin letters
         *            (`[a-z]`), numbers (`[0-9]`), and underscores (`[_]`).
         */
        const std::string id;

        /**
         * \brief The function which handles lifecycle updates for this module.
         *
         * This function will accept a single argument of type `const`
         * LifecycleStage and will not return anything.
         *
         * This function should handle initialization of the module when the
         * engine starts, as well as deinitialization when the engine stops.
         *
         * \sa LifecycleStage
         */
        const LifecycleUpdateCallback lifecycle_update_callback;

        /**
         * \brief A list of IDs of modules this one is dependent on.
         *
         * If any dependency fails to load, the dependent module will also fail.
         */
        const std::vector<std::string> dependencies;
    };

    /**
     * \brief Registers a module for use with the engine.
     *
     * This function should be invoked upon the module library being loaded.
     *
     * \attention For convenience, the macro REGISTER_ARGUS_MODULE registers an
     *            entry point which invokes this function automatically.
     *
     * \throw std::invalid_argument If a module with the given ID is already
     *        registered.
     */
    void register_module(const DynamicModule &module);

    /**
     * \brief Enables a registered module on demand.
     *
     * \param module_id The ID of the module to enable.
     *
     * \throw std::invalid_argument If no module with the given ID is currently
     *        registered.
     */
    void enable_module(const std::string &module_id);
}
