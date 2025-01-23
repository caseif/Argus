#pragma once

#include "argus/scripting/error.hpp"
#include "argus/scripting/scripting_language_plugin.hpp"

#include "argus/resman/resource.hpp"

#include "argus/lowlevel/result.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace argus {
    class ScriptManager {
        std::map<std::string, ScriptingLanguagePlugin *> lang_plugins;
        std::map<std::string, std::string> media_type_langs;
        std::map<std::string, BoundTypeDef> bound_types;
        std::map<std::string, std::string> bound_type_ids;
        std::map<std::string, BoundEnumDef> bound_enums;
        std::map<std::string, std::string> bound_enum_ids;
        std::map<std::string, BoundFunctionDef> bound_global_fns;
        std::vector<ScriptContext *> script_contexts;
        // key = language name, value = resources loaded by the corresponding plugin
        std::map<std::string, std::vector<const Resource *>> loaded_resources;

      public:
        [[nodiscard]] static ScriptManager &instance(void);

        ScriptManager(void);

        ScriptManager(const ScriptManager &) = delete;

        ScriptManager(ScriptManager &&) = delete;

        ScriptManager &operator=(const ScriptManager &) = delete;

        ScriptManager &operator=(ScriptManager &&) = delete;

        std::optional<std::reference_wrapper<ScriptingLanguagePlugin>> get_language_plugin(
                const std::string &lang_name);

        std::optional<std::reference_wrapper<ScriptingLanguagePlugin>> get_media_type_plugin(
            const std::string &media_type);

        void register_language_plugin(ScriptingLanguagePlugin &plugin);

        void unregister_language_plugin(ScriptingLanguagePlugin &plugin);

        [[nodiscard]] Result<Resource &, ScriptLoadError> load_resource(
            const std::string &lang_name,
            const std::string &uid
        );

        void move_resource(const std::string &lang_name, const Resource &resource);

        void release_resource(const std::string &lang_name, const Resource &resource);

        [[nodiscard]] Result<void, BindingError> bind_type(const BoundTypeDef &def);

        [[nodiscard]] Result<void, BindingError> bind_enum(const BoundEnumDef &def);

        [[nodiscard]] Result<void, BindingError> bind_global_function(const BoundFunctionDef &def);

        [[nodiscard]] Result<const BoundTypeDef &, BindingError> get_bound_type_by_name(
                const std::string &type_name) const;

        [[nodiscard]] Result<const BoundTypeDef &, BindingError> get_bound_type_by_type_id(
                const std::string &type_id) const;

        template<typename T>
        [[nodiscard]] Result<const BoundTypeDef &, BindingError> get_bound_type(void) const {
            return get_bound_type_by_type_id(
                typeid(std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>).name()
            );
        }

        [[nodiscard]] Result<const BoundEnumDef &, BindingError> get_bound_enum_by_name(
                const std::string &enum_name) const;

        [[nodiscard]] Result<const BoundEnumDef &, BindingError> get_bound_enum_by_type_id(
                const std::string &enum_type_id) const;

        template<typename T>
        [[nodiscard]] Result<const BoundEnumDef &, BindingError> get_bound_enum(void) const {
            return get_bound_enum_by_type_id(typeid(std::remove_const_t<T>).name());
        }

        [[nodiscard]] Result<void, BindingError> apply_bindings_to_context(ScriptContext &context) const;

        [[nodiscard]] Result<void, BindingError> apply_bindings_to_all_contexts(void) const;

        Result<const BoundFunctionDef &, SymbolNotBoundError> get_native_global_function(const std::string &name) const;

        Result<const BoundFunctionDef &, SymbolNotBoundError> get_native_member_instance_function(
                const std::string &type_name, const std::string &fn_name) const;

        Result<const BoundFunctionDef &, SymbolNotBoundError> get_native_extension_function(
                const std::string &type_name, const std::string &fn_name) const;

        Result<const BoundFunctionDef &, SymbolNotBoundError> get_native_member_static_function(
                const std::string &type_name, const std::string &fn_name) const;

        Result<const BoundFieldDef &, SymbolNotBoundError> get_native_member_field(const std::string &type_name,
                const std::string &field_name) const;

        void register_context(ScriptContext &context);

        void unregister_context(ScriptContext &context);

        Result<void, BindingError> resolve_types(void);

        void perform_deinit(void);
    };
}
