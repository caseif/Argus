/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/filesystem.hpp"
#include "argus/threading.hpp"
#include "argus/time.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core/config.hpp"
#include "internal/core/core_util.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#define US_PER_S 1000000LLU
#define SLEEP_OVERHEAD_NS 120000LLU

#define MODULES_DIR_NAME "modules"
#ifdef _WIN32
#define SHARED_LIB_EXT "dll"
#elif defined(__APPLE__)
#define SHARED_LIB_EXT "dylib"
#else
#define SHARED_LIB_EXT "so"
#endif

namespace argus {

    struct ArgusEventHandler {
        ArgusEventType type;
        ArgusEventCallback callback;
        void *data;
    };

    // This struct defines the list alongside two mutation queues and a shared
    // mutex. In this way, it facilitates a thread-safe callback list wherein
    // the callbacks themselves may modify the list, i.e. while the list is
    // being iterated.
    template<typename T>
    struct CallbackList {
        std::vector<IndexedValue<T>> list;
        std::queue<IndexedValue<T>> addition_queue;
        std::queue<Index> removal_queue;
        SharedMutex list_mutex;
        SharedMutex queue_mutex;
    };

    Thread *g_render_thread;

    static Index g_next_index = 0;
    static std::mutex g_next_index_mutex;

    static CallbackList<DeltaCallback> g_update_callbacks;
    static CallbackList<DeltaCallback> g_render_callbacks;
    static CallbackList<ArgusEventHandler> g_event_listeners;

    static std::queue<std::unique_ptr<ArgusEvent>> g_event_queue;
    static std::mutex g_event_queue_mutex;

    static std::map<const std::string, const ArgusModule> g_registered_modules;
    static std::set<ArgusModule, bool(*)(const ArgusModule, const ArgusModule)> g_enabled_modules(
        [](const ArgusModule a, const ArgusModule b) {
            if (a.layer != b.layer) {
                return a.layer < b.layer;
            } else {
                return a.id.compare(b.id) < 0;
            }
        }
    );

    static std::vector<void*> g_external_module_handles;

    static bool g_engine_stopping = false;

    extern EngineConfig g_engine_config;

    bool g_initializing = false;
    bool g_initialized = false;

    // module lifecycle hooks
    void init_module_core(void);
    extern void init_module_ecs(void);
    extern void init_module_input(void);
    extern void init_module_resman(void);
    extern void init_module_render(void);

    std::map<const std::string, const NullaryCallback> g_stock_module_initializers{
        {MODULE_CORE, init_module_core},
        {MODULE_ECS, init_module_ecs},
        {MODULE_INPUT, init_module_input},
        {MODULE_RESMAN, init_module_resman},
        {MODULE_RENDER, init_module_render}
    };

    static void _interrupt_handler(int signal) {
        stop_engine();
    }

    static void _handle_idle(const Timestamp start_timestamp, const unsigned int target_rate) {
        if (target_rate == 0) {
            return;
        }

        TimeDelta delta = argus::microtime() - start_timestamp;

        unsigned int frametime_target_us = US_PER_S / target_rate;
        if (delta < frametime_target_us) {
            unsigned long long sleep_time_ns = (frametime_target_us - delta) * 1000;
            if (sleep_time_ns <= SLEEP_OVERHEAD_NS) {
                return;
            }
            sleep_nanos(sleep_time_ns - SLEEP_OVERHEAD_NS);
        }
    }

    static const TimeDelta _compute_delta(Timestamp &last_timestamp) {
        TimeDelta delta;

        if (last_timestamp != 0) {
            delta = argus::microtime() - last_timestamp;
        } else {
            delta = 0;
        }
        last_timestamp = argus::microtime();

        return delta;
    }

    template<typename T>
    static const bool _remove_from_indexed_vector(std::vector<IndexedValue<T>> &vector, const Index id) {
        auto it = std::remove_if(vector.begin(), vector.end(),
                [id](auto callback) {return callback.id == id;});
        if (it != vector.end()) {
            vector.erase(it, vector.end());
            return true;
        }
        return false;
    }

    template<typename T>
    static void _flush_callback_list_queues(CallbackList<T> &list) {
        list.queue_mutex.lock_shared();

        // avoid acquiring an exclusive lock unless we actually need to update the list
        if (!list.removal_queue.empty()) {
            list.queue_mutex.unlock_shared(); // VC++ doesn't allow upgrading lock ownership
            // it's important that we lock list_mutex first, since the callback loop has a perpetual lock on it
            // and individual callbacks may invoke _unregister_callback (thus locking queue_mutex).
            // failure to follow this order will cause deadlock.
            list.list_mutex.lock(); // we need to get a lock on the list since we're updating it
            list.queue_mutex.lock();
            while (!list.removal_queue.empty()) {
                Index id = list.removal_queue.front();
                list.removal_queue.pop();
                if (!_remove_from_indexed_vector(list.list, id)) {
                    _ARGUS_WARN("Game attempted to unregister unknown callback %llu\n", id);
                }
            }
            list.queue_mutex.unlock();
            list.list_mutex.unlock();
        } else {
            list.queue_mutex.unlock_shared();
        }

        // same here
        list.queue_mutex.lock_shared();
        if (!list.addition_queue.empty()) {
            list.queue_mutex.unlock_shared();
            // same deal with the ordering
            list.list_mutex.lock();
            list.queue_mutex.lock();
            while (!list.addition_queue.empty()) {
                list.list.insert(list.list.cend(), list.addition_queue.front());
                list.addition_queue.pop();
            }
            list.queue_mutex.unlock();
            list.list_mutex.unlock();
        } else {
            list.queue_mutex.unlock_shared();
        }
    }

    static void _process_event_queue(void) {
        g_event_queue_mutex.lock();
        g_event_listeners.list_mutex.lock_shared();

        while (!g_event_queue.empty()) {
            ArgusEvent &event = *std::move(g_event_queue.front().get());
            for (IndexedValue<ArgusEventHandler> listener : g_event_listeners.list) {
                if (static_cast<int>(listener.value.type & event.type)) {
                    listener.value.callback(event, listener.value.data);
                }
            }
            g_event_queue.pop();
        }

        g_event_listeners.list_mutex.unlock_shared();
        g_event_queue_mutex.unlock();
    }

    constexpr inline ArgusEventType operator |(const ArgusEventType lhs, const ArgusEventType rhs) {
        return static_cast<ArgusEventType>(
                static_cast<std::underlying_type<ArgusEventType>::type>(lhs)
                | static_cast<std::underlying_type<ArgusEventType>::type>(rhs)
        );
    }

    constexpr inline ArgusEventType operator |=(const ArgusEventType lhs, const ArgusEventType rhs) {
        return static_cast<ArgusEventType>(
                static_cast<std::underlying_type<ArgusEventType>::type>(lhs)
                | static_cast<std::underlying_type<ArgusEventType>::type>(rhs)
        );
    }

    constexpr inline ArgusEventType operator &(const ArgusEventType lhs, const ArgusEventType rhs) {
        return static_cast<ArgusEventType>(
                static_cast<std::underlying_type<ArgusEventType>::type>(lhs)
                & static_cast<std::underlying_type<ArgusEventType>::type>(rhs)
        );
    }

    void _update_lifecycle_core(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PRE_INIT:
                _ARGUS_ASSERT(!g_initializing && !g_initialized, "Cannot initialize engine more than once.");

                g_initializing = true;

                // we'll probably register around 10 or so internal callbacks, so allocate them now
                g_update_callbacks.list.reserve(10);
                break;
            case LifecycleStage::INIT:
                g_initialized = true;
                break;
            case LifecycleStage::POST_DEINIT:
                g_render_thread->detach();
                g_render_thread->destroy();

                break;
            default:
                break;
        }
    }

    void init_module_core(void) {
        register_module({MODULE_CORE, 1, {}, _update_lifecycle_core});
    }

    void register_module(const ArgusModule module) {
        if (g_registered_modules.find(module.id) != g_registered_modules.cend()) {;
            throw std::invalid_argument("Module is already registered: " + module.id);
        }

        for (char ch : module.id) {
            if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || (ch == '_'))) {
                throw std::invalid_argument("Invalid module identifier: " + module.id);
            }
        }

        g_registered_modules.insert(std::make_pair(module.id, module));

        _ARGUS_INFO("Registered module %s\n", module.id.c_str());
    }

    void _init_stock_modules(void) {
        for (auto it = g_stock_module_initializers.cbegin(); it != g_stock_module_initializers.cend(); it++) {
            it->second();
        }
    }

    void _load_external_modules(void) {
        std::string modules_dir_path = get_parent(get_executable_path()) + PATH_SEPARATOR MODULES_DIR_NAME;

        if (!is_directory(modules_dir_path)) {
            _ARGUS_INFO("No external modules to load.\n");
            return;
        }

        std::vector<std::string> entries = list_directory_entries(modules_dir_path);
        if (entries.empty()) {
            _ARGUS_INFO("No external modules to load.\n");
            return;
        }

        for (std::string filename : entries) {
            std::string full_path = modules_dir_path + PATH_SEPARATOR + filename;

            if (!is_regfile(full_path)) {
                continue;
            }

            std::string ext = "";
            size_t ext_index = filename.rfind(EXTENSION_SEPARATOR);
            if (ext_index != std::string::npos) {
                ext = filename.substr(ext_index + 1);
            }

            if (ext != SHARED_LIB_EXT) {
                _ARGUS_WARN("Not loading file %s as module (bad extension %s)\n",
                        filename.c_str(), ext.empty() ? "(none)" : ext.c_str());
                continue;
            }

            _ARGUS_INFO("Found external module file %s, attempting to load.\n", filename.c_str());

            void *handle;
            #ifdef _WIN32
            handle = LoadLibraryA(full_path.c_str());
            if (handle == nullptr) {
                _ARGUS_WARN("Failed to load external module file %s (errno: %d)\n", filename.c_str(), GetLastError());
                continue;
            }
            #else
            handle = dlopen(full_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
            if (handle == nullptr) {
                _ARGUS_WARN("Failed to load external module file %s (error: %s)\n", filename.c_str(), dlerror());
                continue;
            }
            #endif
            

            g_external_module_handles.insert(g_external_module_handles.begin(), handle);
        }
    }

    void _unload_external_modules(void) {
        for (void *handle : g_external_module_handles) {
            #ifdef _WIN32
            if (FreeLibrary(reinterpret_cast<HMODULE>(handle)) == 0) {
                _ARGUS_WARN("Failed to unload external module (errno: %d)\n", GetLastError());
            }
            #else
            if (dlclose(handle) != 0) {
                _ARGUS_WARN("Failed to unload external module (errno: %d)\n", errno);
            }
            #endif
        }
    }

    void _enable_module(const std::string module_id, const std::vector<std::string> dependent_chain) {
        // skip duplicates
        for (const ArgusModule enabled_module : g_enabled_modules) {
            if (enabled_module.id == module_id) {
                if (dependent_chain.empty()) {
                    _ARGUS_WARN("Module \"%s\" requested more than once.\n", module_id.c_str());
                }
                return;
            }
        }

        auto it = g_registered_modules.find(module_id);
        if (it == g_registered_modules.cend()) {
            std::stringstream err_msg;
            err_msg << "Module \"" << module_id << "\" was requested, but is not registered";
            for (const std::string dependent : dependent_chain) {
                err_msg << "\n    Required by module \"" << dependent << "\"";
            }
            throw std::invalid_argument(err_msg.str());
        }

        std::vector<std::string> new_chain = dependent_chain;
        new_chain.insert(new_chain.cend(), module_id);
        for (const std::string dependency : it->second.dependencies) {
            _enable_module(dependency, new_chain);
        }

        g_enabled_modules.insert(it->second);

        _ARGUS_INFO("Enabled module %s.\n", module_id.c_str());
    }

    void _load_modules(const std::vector<std::string> &modules) {
        for (const std::string module : modules) {
            _enable_module(module, {});
        }
    }

    void _deinitialize_modules(void) {
        for (LifecycleStage stage = LifecycleStage::PRE_DEINIT; stage <= LifecycleStage::POST_DEINIT;
                stage = static_cast<LifecycleStage>(static_cast<uint32_t>(stage) + 1)) {
            for (auto it = g_enabled_modules.rbegin(); it != g_enabled_modules.rend(); it++) {
                it->lifecycle_update_callback(stage);
            }
        }
    }

    void initialize_engine() {
        signal(SIGINT, _interrupt_handler);

        _init_stock_modules();

        _load_external_modules();

        if (g_engine_config.load_modules.size() > 0) {
            _load_modules(g_engine_config.load_modules);
        } else {
            _load_modules({"core"});
        }

        for (ArgusModule module : g_enabled_modules) {
        }

        for (LifecycleStage stage = LifecycleStage::PRE_INIT; stage <= LifecycleStage::POST_INIT;
                stage = static_cast<LifecycleStage>(static_cast<uint32_t>(stage) + 1)) {
            for (auto it = g_enabled_modules.cbegin(); it != g_enabled_modules.cend(); it++) {
                it->lifecycle_update_callback(stage);
            }
        }
    }

    static void _clean_up(void) {
        _deinitialize_modules();

        _unload_external_modules();
    }

    static void *_game_loop(void *const _) {
        static Timestamp last_update = 0;

        while (1) {
            if (g_engine_stopping) {
                _clean_up();
                break;
            }

            Timestamp update_start = argus::microtime();
            TimeDelta delta = _compute_delta(last_update);

            //TODO: should we flush the queues before the engine stops?
            _flush_callback_list_queues(g_update_callbacks);
            _flush_callback_list_queues(g_render_callbacks);
            _flush_callback_list_queues(g_event_listeners);

            _process_event_queue();

            // invoke update callbacks
            g_update_callbacks.list_mutex.lock_shared();
            for (auto &callback : g_update_callbacks.list) {
                callback.value(delta);
            }
            g_update_callbacks.list_mutex.unlock_shared();

            if (g_engine_config.target_tickrate != 0) {
                _handle_idle(update_start, g_engine_config.target_tickrate);
            }
        }

        return nullptr;
    }

    static void _render_loop() {
        static Timestamp last_frame = 0;

        while (1) {
            if (g_engine_stopping) {
                break;
            }

            Timestamp render_start = argus::microtime();
            TimeDelta delta = _compute_delta(last_frame);

            // invoke render callbacks
            g_render_callbacks.list_mutex.lock_shared();
            for (auto &callback : g_render_callbacks.list) {
                callback.value(delta);
            }
            g_render_callbacks.list_mutex.unlock_shared();

            if (g_engine_config.target_framerate != 0) {
                _handle_idle(render_start, g_engine_config.target_framerate);
            }
        }
    }

    template<typename T>
    Index _add_callback(CallbackList<T> &list, T callback) {
        g_next_index_mutex.lock();
        Index index = g_next_index++;
        g_next_index_mutex.unlock();

        list.queue_mutex.lock();
        list.addition_queue.push({index, callback});
        list.queue_mutex.unlock();

        return index;
    }

    template<typename T>
    void _remove_callback(CallbackList<T> &list, const Index index) {
        list.queue_mutex.lock();
        list.removal_queue.push(index);
        list.queue_mutex.unlock();
    }

    const Index register_update_callback(const DeltaCallback callback) {
        _ARGUS_ASSERT(g_initializing || g_initialized, "Cannot register update callback before engine initialization.");
        return _add_callback(g_update_callbacks, callback);
    }

    void unregister_update_callback(const Index id) {
        _remove_callback(g_update_callbacks, id);
    }

    const Index register_render_callback(const DeltaCallback callback) {
        _ARGUS_ASSERT(g_initializing || g_initialized, "Cannot register render callback before engine initialization.");
        return _add_callback(g_render_callbacks, callback);
    }

    void unregister_render_callback(const Index id) {
        _remove_callback(g_render_callbacks, id);
    }

    const Index register_event_handler(const ArgusEventType type, const ArgusEventCallback callback, void *const data) {
        _ARGUS_ASSERT(g_initializing || g_initialized, "Cannot register event listener before engine initialization.");
        Index id = g_next_index++;
        _ARGUS_ASSERT(callback != nullptr, "Event listener cannot have null callback.");

        ArgusEventHandler listener = {type, callback, data};
        return _add_callback(g_event_listeners, listener);
    }

    void unregister_event_handler(const Index id) {
        _remove_callback(g_event_listeners, id);
    }

    void _dispatch_event_ptr(std::unique_ptr<ArgusEvent> &&event) {
        g_event_queue.push(std::move(event));
    }

    void start_engine(const DeltaCallback game_loop) {
        _ARGUS_ASSERT(g_initialized, "Cannot start engine before it is initialized.");
        _ARGUS_ASSERT(game_loop != NULL, "start_engine invoked with null callback");

        register_update_callback(game_loop);

        g_render_thread = &Thread::create(_game_loop, nullptr);

        // pass control over to the render loop
        _render_loop();

        exit(0);
    }

    void stop_engine(void) {
        _ARGUS_ASSERT(g_initialized, "Cannot stop engine before it is initialized.");

        g_engine_stopping = true;
    }

    ArgusEvent::ArgusEvent(ArgusEventType type):
            type(type) {
    }

}
