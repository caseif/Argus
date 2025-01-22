#include "argus/scripting/manager.hpp"
#include "argus/scripting/util.hpp"
#include "internal/scripting/pimpl/script_context.hpp"
#include "internal/scripting/util.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/lowlevel/collections.hpp"

namespace argus {
    static ScriptManager g_instance = ScriptManager();

    ScriptManager &ScriptManager::instance(void) {
        return g_instance;
    }

    ScriptManager::ScriptManager() {
    }

    static Result<const BoundFunctionDef &, SymbolNotBoundError> _get_native_function(
            const ScriptManager &manager, FunctionType fn_type, const std::string &type_name, const std::string &fn_name) {
        switch (fn_type) {
            case FunctionType::MemberInstance:
            case FunctionType::MemberStatic:
            case FunctionType::Extension: {
                auto type_res = manager.get_bound_type_by_name(type_name);
                if (type_res.is_err()) {
                    return err<const BoundFunctionDef &, SymbolNotBoundError>(SymbolType::Type, type_name);
                }

                const auto &fn_map = fn_type == FunctionType::MemberInstance
                        ? type_res.unwrap().instance_functions
                        : fn_type == FunctionType::Extension
                                ? type_res.unwrap().extension_functions
                                : type_res.unwrap().static_functions;

                auto fn_it = fn_map.find(fn_name);
                if (fn_it == fn_map.cend()) {
                    return err<const BoundFunctionDef &, SymbolNotBoundError>(SymbolType::Function,
                            get_qualified_function_name(fn_type, type_name, fn_name));
                }
                const auto &def = fn_it->second;
                return ok<const BoundFunctionDef &, SymbolNotBoundError>(def);
            }
            case FunctionType::Global: {
                argus_assert(false);
            }
            default:
                crash("Unknown function type ordinal %d", fn_type);
        }
    }

    std::optional<std::reference_wrapper<ScriptingLanguagePlugin>> ScriptManager::get_language_plugin(
            const std::string &lang_name) {
        auto it = this->lang_plugins.find(lang_name);
        if (it != this->lang_plugins.end()) {
            return std::make_optional<std::reference_wrapper<ScriptingLanguagePlugin>>(
                std::reference_wrapper(*it->second)
            );
        } else {
            return std::nullopt;
        }
    }

    std::optional<std::reference_wrapper<ScriptingLanguagePlugin>> ScriptManager::get_media_type_plugin(
            const std::string &media_type) {
        auto it = this->media_type_langs.find(media_type);
        if (it != this->media_type_langs.end()) {
            return this->get_language_plugin(it->second);
        } else {
            return std::nullopt;
        }
    }

    void ScriptManager::register_language_plugin(ScriptingLanguagePlugin &plugin) {
        this->lang_plugins.insert({ plugin.get_language_name(), &plugin });
        for (const auto &mt : plugin.get_media_types()) {
            auto existing = this->media_type_langs.find(mt);
            if (existing != this->media_type_langs.cend()) {
                crash("Media type '%s' is already associated with language plugin '%s'",
                        mt.c_str(), existing->second.c_str());
            }
            media_type_langs.insert({ mt, plugin.get_language_name() });
        }
        this->loaded_resources.insert({ plugin.get_language_name(), {} });
    }

    void ScriptManager::unregister_language_plugin(ScriptingLanguagePlugin &plugin) {
        for (auto *res : this->loaded_resources.find(plugin.get_language_name())->second) {
            res->release();
        }
        this->loaded_resources.erase(plugin.get_language_name());
    }

    Result<Resource &, ScriptLoadError> ScriptManager::load_resource(
        const std::string &lang_name,
            const std::string &uid
    ) {
        auto res = ResourceManager::instance().get_resource(uid);
        if (res.is_err()) {
            //TODO: return error result
            if (res.unwrap_err().reason == ResourceErrorReason::NotFound) {
                return err<Resource &, ScriptLoadError>(uid, "Cannot load script (resource does not exist)");
            } else {
                crash_ll("Failed to load script");
            }
        }

        this->loaded_resources.find(lang_name)->second.push_back(&res.unwrap());

        return ok<Resource &, ScriptLoadError>(res.unwrap());
    }

    void ScriptManager::move_resource(const std::string &lang_name, const Resource &resource) {
        this->loaded_resources.find(lang_name)->second.push_back(&resource);
    }

    void ScriptManager::release_resource(const std::string &lang_name, const Resource &resource) {
        resource.release();
        remove_from_vector(this->loaded_resources.find(lang_name)->second, &resource);
    }

    Result<void, BindingError> ScriptManager::bind_type(const BoundTypeDef &def) {
        if (auto it = this->bound_types.find(def.name); it != this->bound_types.cend()) {
            if (it->second.type_id != def.type_id) {
                return err<void, BindingError>(BindingErrorType::DuplicateName, def.name,
                        "Type with same name has already been bound");
            } else {
                Logger::default_logger().debug("Ignoring duplicate definition for type '%s' with same type ID",
                        def.type_id.c_str());
            }
        }

        if (this->bound_global_fns.find(def.name) != this->bound_global_fns.cend()) {
            return err<void, BindingError>(BindingErrorType::ConflictingName, def.name,
                    "Global function with same name as type has already been bound");
        }

        if (this->bound_enums.find(def.name) != this->bound_enums.cend()) {
            return err<void, BindingError>(BindingErrorType::ConflictingName, def.name,
                    "Enum with same name as type has already been bound");
        }

        //TODO: perform validation on member functions

        std::vector<std::string> static_fn_names;
        std::transform(def.static_functions.cbegin(), def.static_functions.cend(),
                std::back_inserter(static_fn_names),
                [](const auto &fn_def) { return fn_def.second.name; });
        if (std::set(static_fn_names.cbegin(), static_fn_names.cend()).size() != static_fn_names.size()) {
            return err<void, BindingError>(BindingErrorType::InvalidMembers, def.name,
                    "Bound script type contains duplicate static function definitions");
        }

        std::vector<std::string> instance_fn_names;
        std::transform(def.instance_functions.cbegin(), def.instance_functions.cend(),
                std::back_inserter(instance_fn_names),
                [](const auto &fn_def) { return fn_def.second.name; });
        std::transform(def.extension_functions.cbegin(), def.extension_functions.cend(),
                std::back_inserter(instance_fn_names),
                [](const auto &fn_def) { return fn_def.second.name; });
        if (std::set(instance_fn_names.cbegin(), instance_fn_names.cend()).size() != instance_fn_names.size()) {
            return err<void, BindingError>(BindingErrorType::InvalidMembers, def.name,
                    "Bound script type contains duplicate instance/extension function definitions");
        }

        this->bound_types.insert({ def.name, def });
        this->bound_type_ids.insert({ def.type_id, def.name });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> ScriptManager::bind_enum(const BoundEnumDef &def) {
        // check for consistency
        std::unordered_set<int64_t> ordinals;
        ordinals.reserve(def.values.size());
        std::transform(def.values.cbegin(), def.values.cend(), std::inserter(ordinals, ordinals.end()),
                [](const auto &kv) { return kv.second; });
        if (ordinals != def.all_ordinals) {
            return err<void, BindingError>(BindingErrorType::InvalidDefinition, def.name,
                    "Enum definition is corrupted");
        }

        if (auto it = this->bound_enums.find(def.name); it != this->bound_enums.cend()) {
            if (it->second.type_id != def.type_id) {
                return err<void, BindingError>(BindingErrorType::DuplicateName, def.name,
                        "Enum with same name has already been bound");
            } else {
                Logger::default_logger().debug("Ignoring duplicate definition for enum '%s' with same type ID",
                        def.name.c_str());
            }
        }

        if (this->bound_types.find(def.name) != this->bound_types.cend()) {
            return err<void, BindingError>(BindingErrorType::ConflictingName, def.name,
                    "Type with same name as enum has already been bound");
        }

        if (this->bound_global_fns.find(def.name) != this->bound_global_fns.cend()) {
            return err<void, BindingError>(BindingErrorType::ConflictingName, def.name,
                    "Global function with same name as enum has already been bound");
        }

        this->bound_enums.insert({ def.name, def });
        this->bound_enum_ids.insert({ def.type_id, def.name });

        return ok<void, BindingError>();
    }

    Result<void, BindingError> ScriptManager::bind_global_function(const BoundFunctionDef &def) {
        if (this->bound_global_fns.find(def.name) != this->bound_global_fns.cend()) {
            return err<void, BindingError>(BindingErrorType::DuplicateName, def.name,
                    "Global function with same name has already been bound");
        }

        if (this->bound_types.find(def.name) != this->bound_types.cend()) {
            return err<void, BindingError>(BindingErrorType::ConflictingName, def.name,
                    "Type with same name as global function has already been bound");
        }

        if (this->bound_enums.find(def.name) != this->bound_enums.cend()) {
            return err<void, BindingError>(BindingErrorType::ConflictingName, def.name,
                    "Enum with same name as global function has already been bound");
        }

        //TODO: perform validation including:
        //  - check that params types aren't garbage
        //  - check that param sizes match types where applicable
        //  - ensure params passed by value are copy-constructible

        this->bound_global_fns.insert({ def.name, def });

        return ok<void, BindingError>();
    }

    Result<const BoundTypeDef &, BindingError> ScriptManager::get_bound_type_by_name(
            const std::string &type_name) const {
        auto it = this->bound_types.find(type_name);
        if (it == this->bound_types.cend()) {
            return err<const BoundTypeDef &, BindingError>(BindingErrorType::UnknownParent, type_name,
                    "Type name is not bound (check binding order and ensure bind_type is called after "
                    "creating type definition)");
        }
        return ok<const BoundTypeDef &, BindingError>(it->second);
    }

    Result<const BoundTypeDef &, BindingError> ScriptManager::get_bound_type_by_type_id(
            const std::string &type_id) const {
        auto index_it = bound_type_ids.find(type_id);
        if (index_it == bound_type_ids.cend()) {
            return err<const BoundTypeDef &, BindingError>(BindingErrorType::UnknownParent, type_id,
                    "Type " + type_id + " is not bound (check binding order "
                    "and ensure bind_type is called after creating type definition)");
        }
        auto type_it = bound_types.find(index_it->second);
        argus_assert(type_it != bound_types.cend());
        return ok<const BoundTypeDef &, BindingError>(type_it->second);
    }

    Result<const BoundEnumDef &, BindingError> ScriptManager::get_bound_enum_by_name(
            const std::string &enum_name) const {
        auto it = this->bound_enums.find(enum_name);
        if (it == this->bound_enums.cend()) {
            return err<const BoundEnumDef &, BindingError>(BindingErrorType::UnknownParent, enum_name,
                    "Enum name is not bound (check binding order and ensure bind_enum "
                    "is called after creating enum definition)");
        }
        return ok<const BoundEnumDef &, BindingError>(it->second);
    }

    Result<const BoundEnumDef &, BindingError> ScriptManager::get_bound_enum_by_type_id(
        const std::string &enum_type_id) const {
        auto index_it = bound_enum_ids.find(enum_type_id);
        if (index_it == bound_enum_ids.cend()) {
            return err<const BoundEnumDef &, BindingError>(BindingErrorType::UnknownParent, enum_type_id,
                    "Enum " + enum_type_id + " is not bound (check binding order and ensure bind_type "
                    "is called after creating type definition)");
        }
        auto enum_it = bound_enums.find(index_it->second);
        argus_assert(enum_it != bound_enums.cend());
        return ok<const BoundEnumDef &, BindingError>(enum_it->second);
    }

    Result<void, BindingError> ScriptManager::apply_bindings_to_context(ScriptContext &context) const {
        for (const auto &[_, type] : this->bound_types) {
            Logger::default_logger().debug("Binding type %s", type.name.c_str());

            context.m_pimpl->plugin->bind_type(context, type);
            Logger::default_logger().debug("Bound type %s", type.name.c_str());
        }

        for (const auto &[_, type] : this->bound_types) {
            Logger::default_logger().debug("Binding functions for type %s", type.name.c_str());

            for (const auto &[_2, type_fn] : type.instance_functions) {
                Logger::default_logger().debug("Binding instance function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());

                context.m_pimpl->plugin->bind_type_function(context, type, type_fn);

                Logger::default_logger().debug("Bound instance function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());
            }

            for (const auto &[_2, type_fn] : type.extension_functions) {
                Logger::default_logger().debug("Binding extension function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());

                context.m_pimpl->plugin->bind_type_function(context, type, type_fn);

                Logger::default_logger().debug("Bound extension function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());
            }

            for (const auto &[_2, type_fn] : type.static_functions) {
                Logger::default_logger().debug("Binding static function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());

                context.m_pimpl->plugin->bind_type_function(context, type, type_fn);

                Logger::default_logger().debug("Bound static function %s::%s",
                        type.name.c_str(), type_fn.name.c_str());
            }

            Logger::default_logger().debug("Bound %zu instance, %zu extension, and %zu static functions for type %s",
                    type.instance_functions.size(), type.extension_functions.size(),
                    type.static_functions.size(), type.name.c_str());
        }

        for (const auto &[_, type] : this->bound_types) {
            Logger::default_logger().debug("Binding fields for type %s", type.name.c_str());

            for (const auto &[_2, type_field] : type.fields) {
                Logger::default_logger().debug("Binding field %s::%s",
                        type.name.c_str(), type_field.m_name.c_str());

                context.m_pimpl->plugin->bind_type_field(context, type, type_field);

                Logger::default_logger().debug("Bound field %s::%s",
                        type.name.c_str(), type_field.m_name.c_str());
            }

            Logger::default_logger().debug("Bound %zu fields for type %s",
                    type.fields.size(), type.name.c_str());
        }

        for (const auto &[_, enum_def] : this->bound_enums) {
            Logger::default_logger().debug("Binding enum %s", enum_def.name.c_str());

            context.m_pimpl->plugin->bind_enum(context, enum_def);

            Logger::default_logger().debug("Bound enum %s", enum_def.name.c_str());
        }

        for (const auto &[_, fn] : this->bound_global_fns) {
            Logger::default_logger().debug("Binding global function %s", fn.name.c_str());

            context.m_pimpl->plugin->bind_global_function(context, fn);

            Logger::default_logger().debug("Bound global function %s", fn.name.c_str());
        }

        return ok<void, BindingError>();
    }

    Result<void, BindingError> ScriptManager::apply_bindings_to_all_contexts(void) const {
        for (auto *context : this->script_contexts) {
            auto res = this->apply_bindings_to_context(*context);
            if (res.is_err()) {
                return res;
            }
        }

        return ok<void, BindingError>();
    }

    Result<const BoundFunctionDef &, SymbolNotBoundError> ScriptManager::get_native_global_function(
            const std::string &name) const {
        auto it = this->bound_global_fns.find(name);
        if (it == this->bound_global_fns.cend()) {
            return err<const BoundFunctionDef &, SymbolNotBoundError>(SymbolType::Function, name);
        }
        return ok<const BoundFunctionDef &, SymbolNotBoundError>(it->second);
    }

    Result<const BoundFunctionDef &, SymbolNotBoundError> ScriptManager::get_native_member_instance_function(
            const std::string &type_name, const std::string &fn_name) const {
        return _get_native_function(*this, FunctionType::MemberInstance, type_name, fn_name);
    }

    Result<const BoundFunctionDef &, SymbolNotBoundError> ScriptManager::get_native_extension_function(const std::string &type_name,
            const std::string &fn_name) const {
        return _get_native_function(*this, FunctionType::Extension, type_name, fn_name);
    }

    Result<const BoundFunctionDef &, SymbolNotBoundError> ScriptManager::get_native_member_static_function(
            const std::string &type_name, const std::string &fn_name) const {
        return _get_native_function(*this, FunctionType::MemberStatic, type_name, fn_name);
    }

    Result<const BoundFieldDef &, SymbolNotBoundError> ScriptManager::get_native_member_field(
            const std::string &type_name, const std::string &field_name) const {
        auto type_res = this->get_bound_type_by_name(type_name);
        if (type_res.is_err()) {
            return err<const BoundFieldDef &, SymbolNotBoundError>(SymbolType::Type, type_name);
        }

        const auto &field_map = type_res.unwrap().fields;

        auto field_it = field_map.find(field_name);
        if (field_it == field_map.cend()) {
            return err<const BoundFieldDef &, SymbolNotBoundError>(SymbolType::Field,
                    get_qualified_field_name(type_name, field_name));
        }
        return ok<const BoundFieldDef &, SymbolNotBoundError>(field_it->second);
    }

    void ScriptManager::register_context(ScriptContext &context) {
        this->script_contexts.push_back(&context);
    }

    void ScriptManager::unregister_context(ScriptContext &context) {
        remove_from_vector(this->script_contexts, &context);
    }



    [[nodiscard]] static Result<void, BindingError> _resolve_type(ScriptManager &mgr, ObjectType &param_def,
            bool check_copyable = true) {
        if (param_def.type == IntegralType::Callback) {
            argus_assert(param_def.callback_type.has_value());
            auto &callback_type = *param_def.callback_type.value();
            for (auto &subparam : callback_type.params) {
                auto sub_res = _resolve_type(mgr, subparam);
                if (sub_res.is_err()) {
                    return sub_res;
                }
            }

            auto ret_res = _resolve_type(mgr, callback_type.return_type);
            if (ret_res.is_err()) {
                return ret_res;
            }

            return ok<void, BindingError>();
        } else if (param_def.type == IntegralType::Vector || param_def.type == IntegralType::VectorRef) {
            argus_assert(param_def.primary_type.has_value());
            auto el_res = _resolve_type(mgr, *param_def.primary_type.value(), false);
            if (el_res.is_err()) {
                return el_res;
            }

            return ok<void, BindingError>();
        } else if (param_def.type == IntegralType::Result) {
            argus_assert(param_def.primary_type.has_value());
            argus_assert(param_def.secondary_type.has_value());
            auto el_res = _resolve_type(mgr, *param_def.primary_type.value(), false)
                    .collate(_resolve_type(mgr, *param_def.secondary_type.value(), false));
            if (el_res.is_err()) {
                return el_res;
            }

            return ok<void, BindingError>();
        } else if (!is_bound_type(param_def.type)) {
            return ok<void, BindingError>();
        }

        argus_assert(param_def.type_id.has_value());
        //argus_assert(!param_def.type_name.has_value());

        std::string type_name;
        if (param_def.type == IntegralType::Enum) {
            auto bound_enum_res = mgr.get_bound_enum_by_type_id(param_def.type_id.value());
            if (bound_enum_res.is_err()) {
                return err<void, BindingError>(BindingErrorType::UnknownParent, param_def.type_id.value(),
                        "Failed to get enum while resolving function parameter");
            }
            type_name = bound_enum_res.unwrap().name;
        } else {
            auto bound_type_res = mgr.get_bound_type_by_type_id(param_def.type_id.value());
            if (bound_type_res.is_ok()) {
                auto &bound_type = bound_type_res.unwrap();

                if (param_def.type == IntegralType::Struct) {
                    if (check_copyable) {
                        if (bound_type.copy_ctor == nullptr) {
                            return err<void, BindingError>(BindingErrorType::Other, bound_type.name,
                                    "Class-typed parameter passed by value with type "
                                            + bound_type.name + " is not copy-constructible");
                        }

                        if (bound_type.move_ctor == nullptr) {
                            return err<void, BindingError>(BindingErrorType::Other, bound_type.name,
                                    "Class-typed parameter passed by value with type "
                                            + bound_type.name + " is not move-constructible");
                        }

                        if (bound_type.dtor == nullptr) {
                            return err<void, BindingError>(BindingErrorType::Other, bound_type.name,
                                    "Class-typed parameter passed by value with type "
                                            + bound_type.name + " is not destructible");
                        }
                    }

                    param_def.size = bound_type.size;
                }

                type_name = bound_type.name;
            } else {
                auto bound_enum_res = mgr.get_bound_enum_by_type_id(param_def.type_id.value());
                if (bound_enum_res.is_ok()) {
                    auto &bound_enum = bound_enum_res.unwrap();
                    type_name = bound_enum.name;
                    param_def.type = IntegralType::Enum;
                    param_def.size = bound_enum.width;
                } else {
                    return err<void, BindingError>(BindingErrorType::UnknownParent, param_def.type_id.value(),
                            "Failed to get type while resolving function parameter");
                }
            }
        }

        param_def.type_name = type_name;

        return ok<void, BindingError>();
    }

    [[nodiscard]] static Result<void, BindingError> _resolve_field(ScriptManager &mgr, ObjectType &field_def) {
        if (field_def.type == IntegralType::Vector || field_def.type == IntegralType::VectorRef) {
            argus_assert(field_def.primary_type.has_value());
            auto el_res = _resolve_field(mgr, *field_def.primary_type.value());
            if (el_res.is_err()) {
                return el_res;
            }

            return ok<void, BindingError>();
        } else if (!is_bound_type(field_def.type)) {
            return ok<void, BindingError>();
        }

        argus_assert(field_def.type_id.has_value());
        //argus_assert(!field_def.type_name.has_value());

        std::string type_name;
        if (field_def.type == IntegralType::Enum) {
            auto bound_enum_res = mgr.get_bound_enum_by_type_id(field_def.type_id.value());
            if (bound_enum_res.is_err()) {
                return err<void, BindingError>(bound_enum_res.unwrap_err());
            }
            type_name = bound_enum_res.unwrap().name;
        } else {
            auto bound_type_res = mgr.get_bound_type_by_type_id(field_def.type_id.value());
            if (bound_type_res.is_ok()) {
                auto &bound_type = bound_type_res.unwrap();

                field_def.is_refable = bound_type.is_refable;

                if (!bound_type.is_refable) {
                    if (bound_type.copy_ctor == nullptr) {
                        return err<void, BindingError>(BindingErrorType::Other, bound_type.name,
                                "Class-typed field with non-AutoCleanupable type " + bound_type.name
                                        + " is not copy-constructible");
                    }

                    if (bound_type.move_ctor == nullptr) {
                        return err<void, BindingError>(BindingErrorType::Other, bound_type.name,
                                "Class-typed field with non-AutoCleanupable type " + bound_type.name
                                        + " is not move-constructible");
                    }

                    if (bound_type.dtor == nullptr) {
                        return err<void, BindingError>(BindingErrorType::Other, bound_type.name,
                                "Class-typed field with non-AutoCleanupable type " + bound_type.name
                                        + " is not destructible");
                    }
                }

                type_name = bound_type.name;
            } else {
                auto bound_enum_res = mgr.get_bound_enum_by_type_id(field_def.type_id.value());
                if (bound_enum_res.is_ok()) {
                    auto &bound_enum = bound_enum_res.unwrap();
                    type_name = bound_enum.name;
                    field_def.type = IntegralType::Enum;
                    field_def.size = bound_enum.width;
                } else {
                    return err<void, BindingError>(bound_type_res.unwrap_err());
                }
            }
        }

        field_def.type_name = type_name;

        return ok<void, BindingError>();
    }

    [[nodiscard]] static Result<void, BindingError> _resolve_function_types(ScriptManager &mgr,
            BoundFunctionDef &fn_def) {
        for (auto &param : fn_def.params) {
            auto param_res = _resolve_type(mgr, param);
            if (param_res.is_err()) {
                return param_res;
            }
        }

        auto ret_res = _resolve_type(mgr, fn_def.return_type);
        if (ret_res.is_err()) {
            return ret_res;
        }

        return ok<void, BindingError>();
    }

    static Result<void, BindingError> _resolve_member_types(ScriptManager &mgr, BoundTypeDef &type_def) {
        for (auto &fn_kv : type_def.instance_functions) {
            auto &fn = fn_kv.second;

            auto params_res = _resolve_function_types(mgr, fn);

            if (params_res.is_err()) {
                return params_res.map_err<BindingError>([&type_def, &fn](const auto &err) {
                    auto qual_name = get_qualified_function_name(fn.type, type_def.name, fn.name);
                    return BindingError { err.type, qual_name, err.msg };
                });
            }
        }

        for (auto &fn_kv : type_def.extension_functions) {
            auto &fn = fn_kv.second;

            auto params_res = _resolve_function_types(mgr, fn);

            if (params_res.is_err()) {
                return params_res.map_err<BindingError>([&type_def, &fn](const auto &err) {
                    auto qual_name = get_qualified_function_name(fn.type, type_def.name, fn.name);
                    return BindingError { err.type, qual_name, err.msg };
                });
            }
        }

        for (auto &fn_kv : type_def.static_functions) {
            auto &fn = fn_kv.second;

            auto params_res = _resolve_function_types(mgr, fn);

            if (params_res.is_err()) {
                return params_res.map_err<BindingError>([&type_def, &fn](const auto &err) {
                    auto qual_name = get_qualified_function_name(fn.type, type_def.name, fn.name);
                    return BindingError { err.type, qual_name, err.msg };
                });
            }
        }

        for (auto &field_kv : type_def.fields) {
            auto &field = field_kv.second;

            auto field_res = _resolve_field(mgr, field.m_type);

            if (field_res.is_err()) {
                return field_res.map_err<BindingError>([&type_def, &field](const auto &err) {
                    auto qual_name = get_qualified_field_name(type_def.name, field.m_name);
                    return BindingError { err.type, qual_name, err.msg };
                });
            }
        }

        return ok<void, BindingError>();
    }

    Result<void, BindingError> ScriptManager::resolve_types(void) {
        for (auto &[_, type] : this->bound_types) {
            auto res = _resolve_member_types(*this, type);
            if (res.is_err()) {
                return res;
            }
        }

        for (auto &[_, fn] : this->bound_global_fns) {
            auto res = _resolve_function_types(*this, fn);
            if (res.is_err()) {
                return res;
            }
        }

        return ok<void, BindingError>();
    }

    void ScriptManager::perform_deinit(void) {
        for (auto *context : this->script_contexts) {
            auto &plugin = *context->m_pimpl->plugin;
            auto *data = context->m_pimpl->plugin_data;
            if (data != nullptr) {
                plugin.destroy_context_data(data);
            }
        }
        this->script_contexts.clear();

        for (const auto &[_, plugin] : this->lang_plugins) {
            delete plugin;
        }

        this->lang_plugins.clear();
    }
}
