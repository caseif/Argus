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

#include "argus/core/module.hpp"

#include "argus/scripting.hpp"
#include "internal/scripting/bind.hpp"
#include "internal/scripting/module_scripting.hpp"
#include "internal/scripting/pimpl/script_context.hpp"

#include <typeindex>

namespace argus {
    std::map<std::string, ScriptingLanguagePlugin *> g_lang_plugins;
    std::map<std::string, std::string> g_media_type_langs;
    std::map<std::string, BoundTypeDef> g_bound_types;
    std::map<std::type_index, std::string> g_bound_type_indices;
    std::map<std::string, BoundFunctionDef> g_bound_global_fns;
    std::map<std::string, BoundEnumDef> g_bound_enums;
    std::map<std::type_index, std::string> g_bound_enum_indices;
    std::vector<ScriptContext*> g_script_contexts;
    std::map<std::string, std::vector<const Resource*>> g_loaded_resources;

    static void _resolve_all_parameter_types(void) {
        for (auto &type : g_bound_types) {
            resolve_parameter_types(type.second);
        }

        for (auto &fn : g_bound_global_fns) {
            resolve_parameter_types(fn.second);
        }
    }

    static void _bind_types_to_plugins(void) {
        for (const auto &type : g_bound_types) {
            Logger::default_logger().debug("Binding type %s in %zu context%s",
                    type.second.name.c_str(), g_script_contexts.size(),
                    g_script_contexts.size() != 1 ? "s" : "");

            for (auto *context : g_script_contexts) {
                context->pimpl->plugin->bind_type(*context, type.second);
            }
            Logger::default_logger().debug("Bound type %s in %zu context%s",
                    type.second.name.c_str(), g_script_contexts.size(),
                    g_script_contexts.size() != 1 ? "s" : "");
        }
    }

    static void _bind_functions_to_plugins(void) {
        for (const auto &type : g_bound_types) {
            Logger::default_logger().debug("Binding functions for type %s in %zu context%s",
                    type.second.name.c_str(), g_script_contexts.size(),
                    g_script_contexts.size() != 1 ? "s" : "");

            for (const auto &type_fn : type.second.instance_functions) {
                Logger::default_logger().debug("Binding instance function %s::%s",
                        type.second.name.c_str(), type_fn.second.name.c_str());

                for (auto *context : g_script_contexts) {
                    context->pimpl->plugin->bind_type_function(*context, type.second, type_fn.second);
                }

                Logger::default_logger().debug("Bound instance function %s::%s",
                        type.second.name.c_str(), type_fn.second.name.c_str());
            }

            for (const auto &type_fn : type.second.static_functions) {
                Logger::default_logger().debug("Binding static function %s::%s",
                        type.second.name.c_str(), type_fn.second.name.c_str());

                for (auto *context : g_script_contexts) {
                    context->pimpl->plugin->bind_type_function(*context, type.second, type_fn.second);
                }

                Logger::default_logger().debug("Bound static function %s::%s",
                        type.second.name.c_str(), type_fn.second.name.c_str());
            }

            Logger::default_logger().debug("Bound %zu instance and %zu static functions for type %s in %zu context%s",
                    type.second.instance_functions.size(), type.second.static_functions.size(),
                    type.second.name.c_str(), g_script_contexts.size(),
                    g_script_contexts.size() != 1 ? "s" : "");
        }

        for (const auto &enum_def : g_bound_enums) {
            Logger::default_logger().debug("Binding enum %s in %zu context%s",
                    enum_def.second.name.c_str(), g_script_contexts.size(),
                    g_script_contexts.size() != 1 ? "s" : "");

            for (auto *context : g_script_contexts) {
                context->pimpl->plugin->bind_enum(*context, enum_def.second);
            }

            Logger::default_logger().debug("Bound enum %s in %zu context%s",
                    enum_def.second.name.c_str(), g_script_contexts.size(),
                    g_script_contexts.size() != 1 ? "s" : "");
        }

        for (const auto &fn : g_bound_global_fns) {
            Logger::default_logger().debug("Binding global function %s in %zu context%s",
                    fn.second.name.c_str(), g_script_contexts.size(),
                    g_script_contexts.size() != 1 ? "s" : "");

            for (auto *context : g_script_contexts) {
                context->pimpl->plugin->bind_global_function(*context, fn.second);
            }

            Logger::default_logger().debug("Bound global function %s in %zu context%s",
                    fn.second.name.c_str(), g_script_contexts.size(),
                    g_script_contexts.size() != 1 ? "s" : "");
        }
    }

    // temporary testing stuff

    struct Adder {
        int i = 0;

        Adder(int i) : i(i) {}

        Adder(const Adder &lhs) {
            this->i = lhs.i;
        }

        int increment() {
            i += 1;
            return i;
        }

        int add(int j) {
            i += j;
            return i;
        }

        int add_2(const Adder &adder) {
            i += adder.i;
            return i;
        }

        static Adder create(int i) {
            return Adder(i);
        }
    };

    static void _println(const std::string *str) {
        printf("println: %s\n", str->c_str());
    }

    enum Animal {
        Dog,
        Cat,
        Walrus
    };

    static void _animal_noise(Animal animal) {
        switch (animal) {
            case Dog:
                printf("The dog says woof\n");
                break;
            case Cat:
                printf("The cat says meow\n");
                break;
            case Walrus:
                printf("The walrus says arf\n");
                break;
            default:
                printf("I don't know this animal\n");
                break;
        }
    }

    void update_lifecycle_scripting(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Init: {
                auto adder_type = create_type_def<Adder>("Adder");
                add_member_instance_function(adder_type, "increment", &Adder::increment);
                add_member_instance_function(adder_type, "add", &Adder::add);
                add_member_instance_function(adder_type, "add_2", &Adder::add_2);
                add_member_static_function(adder_type, "create", &Adder::create);
                bind_type(adder_type);

                bind_global_function("println", _println);

                auto animal_enum = create_enum_def<Animal>("Animal");
                add_enum_value(animal_enum, "Dog", Animal::Dog);
                add_enum_value(animal_enum, "Cat", Animal::Cat);
                add_enum_value(animal_enum, "Walrus", Animal::Walrus);
                bind_enum(animal_enum);

                bind_global_function("animal_noise", _animal_noise);

                g_script_context = &create_script_context("lua");

                break;
            }
            case LifecycleStage::PostInit: {
                // parameter type resolution is deferred to ensure that all
                // types have been registered first
                _resolve_all_parameter_types();
                _bind_types_to_plugins();
                _bind_functions_to_plugins();

                break;
            }
            case LifecycleStage::Deinit: {
                for (auto context : g_script_contexts) {
                    auto &plugin = *context->pimpl->plugin;
                    auto *data = context->pimpl->plugin_data;
                    if (data != nullptr) {
                        plugin.destroy_context_data(data);
                    }
                }

                for (const auto &plugin : g_lang_plugins) {
                    delete plugin.second;
                }
                break;
            }
            default:
                break;
        }
    }
}
