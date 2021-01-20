/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

#include <cstddef>

#define MODULE_CORE "core"
#define MODULE_WM "wm"
#define MODULE_ECS "ecs"
#define MODULE_INPUT "input"
#define MODULE_RESMAN "resman"
#define MODULE_RENDER "render"

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
#define REGISTER_ARGUS_MODULE(id, layer, dependencies, lifecycle_update_callback) \
    BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) { \
        argus::register_module(argus::ArgusModule{id, layer, dependencies, lifecycle_update_callback}); \
        return true; \
    }
#elif defined(__GNUC__) || defined(__clang__)
#define REGISTER_ARGUS_MODULE(id, layer, dependencies, lifecycle_update_callback) \
    __attribute__((constructor)) void __argus_module_ctor(void) { \
        argus::register_module(argus::ArgusModule{id, layer, dependencies, lifecycle_update_callback}); \
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
        PRE_INIT,
        /**
         * \brief Primary initialization stage, for performing most
         *        initialization tasks.
         */
        INIT,
        /**
         * \brief Post-initialization stage, for performing initialization
         *        contingent on all parent modules being initialized.
         */
        POST_INIT,
        /**
         * \brief Early de-initialization. This occurs immediately after the
         *        engine has committed to shutting down.
         *
         * Should be used for performing early de-initialization tasks, such as
         * saving user data. Changes during this stage should not be visible to
         * dependent modules.
         */
        PRE_DEINIT,
        /**
         * \brief Primary de-initialization.
         *
         * Should be used for performing most de-initialization tasks.
         */
        DEINIT,
        /**
         * \brief Very late initialization.
         *
         * Should be used for performing de-init contingent on parent modules
         * being fully de-initialized as well as for final deallocation and
         * similar tasks.
         */
        POST_DEINIT
    };

    /**
     * \brief A callback for passing lifecycle changes to engine modules.
     */
    typedef std::function<void(const LifecycleStage)> LifecycleUpdateCallback;

    /**
     * \brief Represents a module for the Argus engine.
     *
     * This struct contains all information required to initialize and update
     * the module appropriately.
     */
    struct ArgusModule {
        /**
         * \brief The ID of the module.
         *
         * \attention This ID must contain only lowercase Latin letters
         *            (`[a-z]`), numbers (`[0-9]`), and underscores (`[_]`).
         */
        const std::string id;
        /**
         * \brief The layer of the module.
         *
         * A module may be dependent on another module only if the dependency
         * specifies a lower layer than the dependent. For example, a module
         * on with layer `3` may depend on a module on layer `2`, but not on one
         * on `3` or `4`. This requirements removes any possibility of circular
         * dependencies by necessitating a well-defined load order.
         */
        const uint8_t layer;
        /**
         * \brief A list of IDs of modules this one is dependent on.
         *
         * If any dependency fails to load, the dependent module will also fail.
         */
        const std::vector<std::string> dependencies;
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
    void register_module(const ArgusModule module);

    /**
     * \brief Enables a registered module on demand.
     *
     * \param module_id The ID of the module to enable.
     *
     * \throw std::invalid_argument If no module with the given ID is currently
     *        registered.
     */
    void enable_module(const std::string module_id);
}
