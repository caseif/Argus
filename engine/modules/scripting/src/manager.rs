use crate::bind::{BoundEnumDef, BoundFieldDef, BoundFunctionDef, BoundTypeDef};
use crate::context::{ScriptContext, ScriptContextHandle};
use crate::error::*;
use crate::plugin::ScriptLanguagePlugin;
use crate::util::{get_qualified_field_name, get_qualified_function_name, is_bound_type};
use crate::LOGGER;
use argus_logging::debug;
use argus_scripting_bind::{FunctionType, IntegralType, ObjectType};
use fragile::Fragile;
use std::collections::{HashMap, HashSet};
use std::ops::Deref;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{LazyLock, Mutex, MutexGuard};
use dashmap::DashMap;
use dashmap::mapref::one::{Ref, RefMut};
use argus_core::{get_current_lifecycle_stage, LifecycleStage};
use argus_resman::{Resource, ResourceErrorReason, ResourceManager};

static INSTANCE: LazyLock<ScriptManager> = LazyLock::new(|| ScriptManager::new());

pub struct ScriptManager {
    lang_plugins: DashMap<String, Box<dyn ScriptLanguagePlugin>>,
    media_type_langs: DashMap<String, String>,
    bindings: Mutex<ScriptBindings>,
    // key = language name, value = resources loaded by the corresponding plugin
    loaded_resources: DashMap<String, Vec<Fragile<Resource>>>,
    next_context_handle: AtomicU64,
    script_contexts: DashMap<ScriptContextHandle, Fragile<ScriptContext>>,
}

#[derive(Debug, Default)]
pub struct ScriptBindings {
    types: HashMap<String, BoundTypeDef>,
    type_ids: HashMap<String, String>,
    enums: HashMap<String, BoundEnumDef>,
    enum_ids: HashMap<String, String>,
    global_fns: HashMap<String, BoundFunctionDef>,
}

impl ScriptManager {
    pub(crate) fn new() -> Self {
        Self {
            lang_plugins: Default::default(),
            media_type_langs: Default::default(),
            bindings: Default::default(),
            loaded_resources: Default::default(),
            next_context_handle: Default::default(),
            script_contexts: Default::default(),
        }
    }
}

impl ScriptManager {
    pub fn instance() -> &'static Self {
        &INSTANCE
    }

    pub fn get_language_plugin(
        &self,
        lang_name: impl AsRef<str>,
    ) -> Option<Ref<String, Box<dyn ScriptLanguagePlugin>>> {
        Some(self.lang_plugins.get(lang_name.as_ref())?)
    }

    pub fn get_language_plugin_mut(
        &self,
        lang_name: impl AsRef<str>,
    ) -> Option<RefMut<String, Box<dyn ScriptLanguagePlugin>>> {
        Some(self.lang_plugins.get_mut(lang_name.as_ref())?)
    }
    
    pub fn get_media_type_language(
        &self,
        media_type: impl AsRef<str>,
    ) -> Option<String> {
        self.media_type_langs.get(media_type.as_ref()).map(|s| s.deref().clone())
    }

    pub fn get_media_type_plugin(
        &self,
        media_type: impl AsRef<str>,
    ) -> Option<Ref<String, Box<dyn ScriptLanguagePlugin>>> {
        self.get_language_plugin(self.get_media_type_language(media_type)?)
    }

    pub fn get_media_type_plugin_mut(
        &mut self,
        media_type: impl AsRef<str>,
    ) -> Option<RefMut<String, Box<dyn ScriptLanguagePlugin>>> {
        let lang = self.media_type_langs.get(media_type.as_ref())?.clone();
        self.get_language_plugin_mut(lang)
    }

    pub fn register_language_plugin<P: ScriptLanguagePlugin + 'static>(&self, plugin: P) {
        self.lang_plugins
            .insert(P::get_language_name().to_owned(), Box::new(plugin));
        for &mt in P::get_media_types() {
            if let Some(existing) = self.media_type_langs.get(mt) {
                panic!(
                    "Media type '{}' is already associated with language plugin '{}'",
                    mt,
                    *existing,
                );
            }
            self.media_type_langs
                .insert(mt.into(), P::get_language_name().to_owned());
        }
        self.loaded_resources
            .insert(P::get_language_name().to_owned(), vec![]);
    }

    pub fn unregister_language_plugin<P: ScriptLanguagePlugin>(&self) {
        self.loaded_resources.remove(P::get_language_name());
    }

    pub fn get_bindings(&self) -> MutexGuard<ScriptBindings> {
        self.bindings.lock().unwrap()
    }

    pub fn load_resource(
        &self,
        lang_name: impl AsRef<str>,
        uid: impl AsRef<str>,
    ) -> Result<Resource, ScriptLoadError> {
        let res = ResourceManager::instance().get_resource(uid.as_ref())
            .map_err(|err| {
                //TODO: return error result
                let msg = if err.reason == ResourceErrorReason::NotFound {
                    "Cannot load script (resource does not exist)"
                } else {
                    "Cannot load script (unknown error)"
                };
                ScriptLoadError::new(
                    uid.as_ref(),
                    msg,
                )
            })?;

        self.loaded_resources
            .get_mut(lang_name.as_ref())
            .unwrap()
            .push(Fragile::new(res.clone()));

        Ok(res)
    }

    pub fn move_resource(&self, lang_name: impl AsRef<str>, resource: Resource) {
        self.loaded_resources
            .entry(lang_name.as_ref().to_owned())
            .or_default()
            .push(Fragile::new(resource));
    }

    pub fn release_resource(&self, lang_name: impl AsRef<str>, resource: Resource) {
        self.loaded_resources
            .get_mut(lang_name.as_ref())
            .unwrap()
            .retain(|res| res.get().get_prototype().uid != resource.get_prototype().uid);
    }

    pub fn apply_bindings_to_context(
        &self,
        handle: ScriptContextHandle,
    ) -> Result<(), BindingError> {
        let mut context = self.script_contexts.get_mut(&handle).unwrap();

        let mut plugin = self.lang_plugins.get_mut(&context.get().language).unwrap();

        let bindings = self.bindings.lock().unwrap();

        for ty in bindings.types.values() {
            debug!(LOGGER, "Binding type {}", ty.name);
            plugin.bind_type(context.get_mut(), ty);
        }

        for enum_def in bindings.enums.values() {
            debug!(LOGGER, "Binding enum {}", enum_def.name);

            plugin.bind_enum(context.get_mut(), enum_def);
        }

        for fn_def in bindings.global_fns.values() {
            debug!(LOGGER, "Binding global function {}", fn_def.name);

            plugin.bind_global_function(context.get_mut(), fn_def);
        }

        Ok(())
    }

    pub fn apply_bindings_to_all_contexts(&self) -> Result<(), BindingError> {
        for handle in self.script_contexts.iter().map(|kv| *kv.key()).collect::<Vec<_>>() {
            self.apply_bindings_to_context(handle)?;
        }

        Ok(())
    }

    pub fn register_context(&self, context: ScriptContext) -> ScriptContextHandle {
        let handle = self.next_context_handle.fetch_add(1, Ordering::Relaxed);

        self.script_contexts.insert(handle, Fragile::new(context));

        if get_current_lifecycle_stage() >= LifecycleStage::PostInit {
            self.apply_bindings_to_context(handle)
                .expect("Failed to apply bindings to script context");
        }

        handle
    }

    pub fn unregister_context(&self, handle: ScriptContextHandle) {
        match self.script_contexts.remove(&handle) {
            Some(_) => {}
            None => panic!("No script context with handle {} is registered", handle),
        }
    }

    pub fn get_context(&self, handle: ScriptContextHandle)
        -> Ref<ScriptContextHandle, Fragile<ScriptContext>> {
        self.script_contexts.get(&handle).unwrap()
    }

    pub fn get_context_mut(&self, handle: ScriptContextHandle)
        -> RefMut<ScriptContextHandle, Fragile<ScriptContext>> {
        self.script_contexts.get_mut(&handle).unwrap()
    }

    pub fn perform_deinit(&self) {
        self.script_contexts.clear();
        self.lang_plugins.clear();
    }
}

impl ScriptBindings {
    pub fn bind_type(&mut self, def: BoundTypeDef) -> Result<(), BindingError> {
        if let Some(existing) = self.types.get(&def.name) {
            if existing.type_id != def.type_id {
                return Err(BindingError::new(
                    BindingErrorType::DuplicateName,
                    &def.name,
                    "Type with same name has already been bound",
                ));
            } else {
                debug!(
                    LOGGER,
                    "Ignoring duplicate definition for type '{}' with same type ID", def.type_id
                );
            }
        }

        if self.global_fns.contains_key(&def.name) {
            return Err(BindingError::new(
                BindingErrorType::ConflictingName,
                &def.name,
                "Global function with same name as type has already been bound",
            ));
        }

        if self.enums.contains_key(&def.name) {
            return Err(BindingError::new(
                BindingErrorType::ConflictingName,
                &def.name,
                "Enum with same name as type has already been bound",
            ));
        }

        //TODO: perform validation on member functions

        let unique_static_fn_names = def.static_functions.values()
            .map(|f| &f.name)
            .collect::<HashSet<_>>()
            .len();
        if unique_static_fn_names != def.static_functions.len() {
            return Err(BindingError::new(
                BindingErrorType::InvalidMembers,
                &def.name,
                "Bound script type contains duplicate static function definitions",
            ));
        }

        let unique_instance_fn_names = def
            .instance_functions
            .iter()
            .chain(def.extension_functions.iter())
            .chain(def.static_functions.iter())
            .map(|(_, f)| &f.name)
            .collect::<HashSet<_>>()
            .len();
        if unique_instance_fn_names != def.static_functions.len() + def.instance_functions.len() {
            return Err(BindingError::new(
                BindingErrorType::InvalidMembers,
                &def.name,
                "Bound script type contains duplicate instance/extension function definitions",
            ));
        }

        self.type_ids
            .insert(def.type_id.clone(), def.name.clone());
        self.types.insert(def.name.clone(), def);

        Ok(())
    }

    pub fn bind_enum(&mut self, def: BoundEnumDef) -> Result<(), BindingError> {
        // check for consistency
        let ordinals: HashSet<i64> = def.values.iter().map(|(_, &v)| v).collect();
        //ordinals.dedup();
        if ordinals != def.all_ordinals {
            return Err(BindingError::new(
                BindingErrorType::InvalidDefinition,
                &def.name,
                "Enum definition is corrupted",
            ));
        }

        if let Some(existing) = self.enums.get(&def.name) {
            if existing.type_id != def.type_id {
                return Err(BindingError::new(
                    BindingErrorType::DuplicateName,
                    &def.name,
                    "Enum with same name has already been bound",
                ));
            } else {
                debug!(
                    LOGGER,
                    "Ignoring duplicate definition for enum '{}' with same type ID", def.name
                );
            }
        }

        if self.types.contains_key(&def.name) {
            return Err(BindingError::new(
                BindingErrorType::ConflictingName,
                &def.name,
                "Type with same name as enum has already been bound",
            ));
        }

        if self.global_fns.contains_key(&def.name) {
            return Err(BindingError::new(
                BindingErrorType::ConflictingName,
                &def.name,
                "Global function with same name as enum has already been bound",
            ));
        }

        self.enum_ids
            .insert(def.type_id.clone(), def.name.clone());
        self.enums.insert(def.name.clone(), def);

        Ok(())
    }

    pub fn bind_global_function(&mut self, def: BoundFunctionDef) -> Result<(), BindingError> {
        if self.global_fns.contains_key(&def.name) {
            return Err(BindingError::new(
                BindingErrorType::DuplicateName,
                &def.name,
                "Global function with same name has already been bound",
            ));
        }

        if self.types.contains_key(&def.name) {
            return Err(BindingError::new(
                BindingErrorType::ConflictingName,
                &def.name,
                "Type with same name as global function has already been bound",
            ));
        }

        if self.enums.contains_key(&def.name) {
            return Err(BindingError::new(
                BindingErrorType::ConflictingName,
                def.name,
                "Enum with same name as global function has already been bound",
            ));
        }

        //TODO: perform validation including:
        //  - check that params types aren't garbage
        //  - check that param sizes match types where applicable
        //  - ensure params passed by value are copy-constructible

        self.global_fns.insert(def.name.clone(), def);

        Ok(())
    }

    pub fn get_type_by_name(
        &self,
        type_name: impl AsRef<str>,
    ) -> Result<&BoundTypeDef, BindingError> {
        match self.types.get(type_name.as_ref()) {
            Some(ty) => Ok(ty),
            None => Err(BindingError::new(
                BindingErrorType::UnknownParent,
                type_name.as_ref(),
                "Type name is not bound (check binding order and ensure bind_type is called
                 after creating type definition)",
            )),
        }
    }

    pub fn get_type_by_type_id(
        &self,
        type_id: impl AsRef<str>,
    ) -> Result<&BoundTypeDef, BindingError> {
        if let Some(type_name) = self.type_ids.get(type_id.as_ref()) {
            let bound_type = self.types.get(type_name).unwrap();
            Ok(bound_type)
        } else {
            Err(BindingError::new(
                BindingErrorType::UnknownParent,
                type_id.as_ref(),
                format!(
                    "Type {} is not bound (check binding order and ensure bind_type is called\
                     after creating type definition)",
                    type_id.as_ref(),
                ),
            ))
        }
    }

    pub fn get_enum_by_name(
        &self,
        enum_name: impl AsRef<str>,
    ) -> Result<&BoundEnumDef, BindingError> {
        match self.enums.get(enum_name.as_ref()) {
            Some(def) => Ok(def),
            None => Err(BindingError::new(
                BindingErrorType::UnknownParent,
                enum_name.as_ref().to_owned(),
                "Enum name is not bound (check binding order and ensure bind_enum
                 is called after creating enum definition)",
            )),
        }
    }

    pub fn get_enum_by_type_id(
        &self,
        enum_type_id: impl AsRef<str>,
    ) -> Result<&BoundEnumDef, BindingError> {
        if let Some(type_name) = self.enum_ids.get(enum_type_id.as_ref()) {
            Ok(self.enums.get(type_name).unwrap())
        } else {
            Err(BindingError::new(
                BindingErrorType::UnknownParent,
                enum_type_id.as_ref(),
                format!(
                    "Enum {} is not bound (check binding order and ensure bind_type \
                     is called after creating type definition)",
                    enum_type_id.as_ref(),
                ),
            ))
        }
    }

    pub fn get_global_function(
        &self,
        name: impl AsRef<str>,
    ) -> Result<&BoundFunctionDef, SymbolNotBoundError> {
        match self.global_fns.get(name.as_ref()) {
            Some(def) => Ok(def),
            None => Err(SymbolNotBoundError::new(
                SymbolType::Function,
                name.as_ref(),
            )),
        }
    }

    pub fn get_member_instance_function(
        &self,
        type_name: impl AsRef<str>,
        fn_name: impl AsRef<str>,
    ) -> Result<&BoundFunctionDef, SymbolNotBoundError> {
        self.get_native_function(FunctionType::MemberInstance, type_name, fn_name)
    }

    pub fn get_extension_function(
        &self,
        type_name: impl AsRef<str>,
        fn_name: impl AsRef<str>,
    ) -> Result<&BoundFunctionDef, SymbolNotBoundError> {
        self.get_native_function(FunctionType::Extension, type_name, fn_name)
    }

    pub fn get_member_static_function(
        &self,
        type_name: impl AsRef<str>,
        fn_name: impl AsRef<str>,
    ) -> Result<&BoundFunctionDef, SymbolNotBoundError> {
        self.get_native_function(FunctionType::MemberStatic, type_name, fn_name)
    }

    pub fn get_field(
        &self,
        type_name: impl AsRef<str>,
        field_name: impl AsRef<str>,
    ) -> Result<&BoundFieldDef, SymbolNotBoundError> {
        let type_res = self.get_type_by_name(type_name.as_ref());
        if type_res.is_err() {
            return Err(SymbolNotBoundError::new(
                SymbolType::Type,
                type_name.as_ref(),
            ));
        }

        let field_map = &type_res.unwrap().fields;

        match field_map.get(field_name.as_ref()) {
            Some(def) => Ok(def),
            None => Err(SymbolNotBoundError::new(
                SymbolType::Field,
                get_qualified_field_name(type_name, field_name.as_ref()),
            )),
        }
    }

    pub fn resolve_types(&mut self) -> Result<(), BindingError> {
        let mut types_cloned = self.types.clone();
        for ty in types_cloned.values_mut() {
            self.resolve_member_types(ty)?;
        }
        self.types = types_cloned;

        let mut global_fns_cloned = self.global_fns.clone();
        for f in global_fns_cloned.values_mut() {
            self.resolve_function_types(f)?;
        }
        self.global_fns = global_fns_cloned;

        Ok(())
    }

    fn resolve_type(
        &self,
        param_def: &mut ObjectType,
        check_copyable: bool,
    ) -> Result<(), BindingError> {
        if param_def.ty == IntegralType::Callback {
            for subparam in param_def
                .callback_info
                .as_mut()
                .unwrap()
                .param_types
                .iter_mut()
            {
                self.resolve_type(subparam, true)?;
            }

            return self.resolve_type(
                &mut param_def.callback_info.as_mut().unwrap().return_type,
                true,
            );
        } else if param_def.ty == IntegralType::Vec || param_def.ty == IntegralType::VecRef {
            let Some(prim_type) = param_def.primary_type.as_mut() else {
                panic!();
            };
            return self.resolve_type(prim_type.as_mut(), false);
        } else if param_def.ty == IntegralType::Result {
            let Some(prim_type) = param_def.primary_type.as_mut() else {
                panic!();
            };
            let Some(sec_type) = param_def.secondary_type.as_mut() else {
                panic!();
            };
            return self
                .resolve_type(prim_type, false)
                .and_then(|()| self.resolve_type(sec_type, false));
        } else if !is_bound_type(param_def.ty) {
            return Ok(());
        }

        let type_name: &str;
        if param_def.ty == IntegralType::Enum {
            let type_id = param_def.type_id.as_ref().unwrap();
            let bound_enum_res = self.get_enum_by_type_id(type_id);
            if bound_enum_res.is_err() {
                return Err(BindingError::new(
                    BindingErrorType::UnknownParent,
                    type_id,
                    "Failed to get enum while resolving function parameter",
                ));
            }
            type_name = &bound_enum_res?.name;
        } else {
            let type_id = if param_def.ty == IntegralType::Reference {
                param_def.primary_type.as_ref().unwrap().type_id.as_ref().unwrap()
            } else {
                param_def.type_id.as_ref().unwrap()
            };

            let bound_type_res = self.get_type_by_type_id(type_id);
            if let Ok(bound_type) = bound_type_res {
                if param_def.ty == IntegralType::Object {
                    if check_copyable {
                        if bound_type.cloner.is_none() {
                            return Err(BindingError::new(
                                BindingErrorType::Other,
                                &bound_type.name,
                                format!(
                                    "Class-typed parameter passed by value with type {}
                                     is not cloneable",
                                    bound_type.name,
                                ),
                            ));
                        }

                        if bound_type.dropper.is_none() {
                            return Err(BindingError::new(
                                BindingErrorType::Other,
                                &bound_type.name,
                                format!(
                                    "Class-typed parameter passed by value with type {}
                                     is not droppable",
                                    bound_type.name,
                                ),
                            ));
                        }
                    }

                    param_def.size = bound_type.size;
                }

                type_name = &bound_type.name;
            } else {
                let bound_enum_res = self.get_enum_by_type_id(type_id);
                if bound_enum_res.is_ok() {
                    let bound_enum = bound_enum_res?;
                    type_name = &bound_enum.name;
                    param_def.ty = IntegralType::Enum;
                    param_def.size = bound_enum.width;
                } else {
                    return Err(bound_type_res.unwrap_err());
                }
            }
        }

        param_def.type_name = Some(type_name.to_owned());

        Ok(())
    }

    fn resolve_field(&self, field_def: &mut ObjectType) -> Result<(), BindingError> {
        if field_def.ty == IntegralType::Vec || field_def.ty == IntegralType::VecRef {
            return self.resolve_field(field_def.primary_type.as_mut().unwrap());
        } else if !is_bound_type(field_def.ty) {
            return Ok(());
        }

        //assert!(!field_def.type_name.has_value());

        let type_id = field_def.type_id.as_ref().unwrap();

        let type_name: &str;
        if field_def.ty == IntegralType::Enum {
            type_name = &self.get_enum_by_type_id(&type_id)
                .map_err(|err| {
                    BindingError::new(
                        BindingErrorType::UnknownParent,
                        err.bound_name,
                        "Failed to get type while resolving member field",
                    )
                })?.name;
        } else {
            let bound_type_res = self.get_type_by_type_id(&type_id);
            if let Ok(bound_type) = bound_type_res {
                field_def.is_refable = Some(bound_type.is_refable);

                if !bound_type.is_refable {
                    if bound_type.cloner.is_none() {
                        return Err(BindingError::new(
                            BindingErrorType::Other,
                            &bound_type.name,
                            format!(
                                "Object-typed field with non-refable type {}
                                 is not cloneable",
                                bound_type.name
                            ),
                        ));
                    }

                    if bound_type.dropper.is_none() {
                        return Err(BindingError::new(
                            BindingErrorType::Other,
                            &bound_type.name,
                            format!(
                                "Class-typed field with non-refable type {}
                                 is not droppable",
                                bound_type.name,
                            ),
                        ));
                    }
                }

                type_name = &bound_type.name;
            } else {
                let bound_enum_res = self.get_enum_by_type_id(type_id);
                if bound_enum_res.is_ok() {
                    let bound_enum = bound_enum_res?;
                    type_name = &bound_enum.name;
                    field_def.ty = IntegralType::Enum;
                    field_def.size = bound_enum.width;
                } else {
                    return Err(bound_type_res.unwrap_err());
                }
            }
        }

        field_def.type_name = Some(type_name.to_owned());

        Ok(())
    }

    fn resolve_function_types(
        &self,
        fn_def: &mut BoundFunctionDef,
    ) -> Result<(), BindingError> {
        for param in &mut fn_def.param_types {
            self.resolve_type(param, true)?;
        }

        self.resolve_type(&mut fn_def.return_type, true)?;

        Ok(())
    }

    fn resolve_member_types(&self, type_def: &mut BoundTypeDef) -> Result<(), BindingError> {
        for fn_list in [
            &mut type_def.instance_functions,
            &mut type_def.extension_functions,
            &mut type_def.static_functions,
        ] {
            for f in fn_list.values_mut() {
                self.resolve_function_types(f).map_err(|err| {
                    let qual_name =
                        get_qualified_function_name(f.ty, &f.name, Some(&type_def.name));
                    BindingError::new(err.ty, qual_name, err.msg)
                })?;
            }
        }

        for field in type_def.fields.values_mut() {
            self.resolve_field(&mut field.ty).map_err(|err| {
                let qual_name = get_qualified_field_name(&type_def.name, &field.name);
                BindingError::new(err.ty, qual_name, err.msg)
            })?;
        }

        Ok(())
    }

    fn get_native_function(
        &self,
        fn_type: FunctionType,
        type_name: impl AsRef<str>,
        fn_name: impl AsRef<str>,
    ) -> Result<&BoundFunctionDef, SymbolNotBoundError> {
        match fn_type {
            FunctionType::MemberInstance | FunctionType::MemberStatic | FunctionType::Extension => {
                let Ok(type_def) = self.get_type_by_name(type_name.as_ref()) else {
                    return Err(SymbolNotBoundError::new(SymbolType::Type, type_name.as_ref()));
                };

                let fn_map = match fn_type {
                    FunctionType::MemberInstance => &type_def.instance_functions,
                    FunctionType::Extension => &type_def.extension_functions,
                    _ => &type_def.static_functions,
                };

                match fn_map.get(fn_name.as_ref()) {
                    Some(fn_def) => Ok(fn_def),
                    None => Err(SymbolNotBoundError::new(
                        SymbolType::Function,
                        get_qualified_function_name(fn_type, &fn_name, Some(&type_def.name)),
                    )),
                }
            }
            FunctionType::Global => {
                panic!()
            }
        }
    }
}
