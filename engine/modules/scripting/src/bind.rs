use std::collections::{HashMap, HashSet};
use std::fmt;
use std::fmt::{Debug, Formatter};
use argus_scripting_bind::*;
use crate::error::{BindingError, BindingErrorType};
use crate::get_qualified_field_name;
use crate::util::get_qualified_function_name;

pub type CloneProxy = fn(*mut (), *const ());
pub type DropProxy = fn(*mut ());

#[derive(Clone)]
pub struct BoundTypeDef {
    pub name: String,
    pub size: usize,
    pub type_id: String,
    // whether references to the type can be passed to scripts
    // (i.e. whether the type derives from AutoCleanupable)
    pub is_refable: bool,
    // the copy and move ctors and dtor are only used for struct value and callback types
    pub cloner: Option<CloneProxy>,
    pub dropper: Option<DropProxy>,
    pub instance_functions: HashMap<String, BoundFunctionDef>,
    pub extension_functions: HashMap<String, BoundFunctionDef>,
    pub static_functions: HashMap<String, BoundFunctionDef>,
    pub fields: HashMap<String, BoundFieldDef>,
}

impl Debug for BoundTypeDef {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result<(), fmt::Error> {
        #[allow(dead_code)]
        #[derive(Debug)]
        struct BoundTypeDef<'a> {
            pub name: &'a String,
            pub size: &'a usize,
            pub type_id: &'a String,
            pub is_refable: &'a bool,
            pub instance_functions: &'a HashMap<String, BoundFunctionDef>,
            pub extension_functions: &'a HashMap<String, BoundFunctionDef>,
            pub static_functions: &'a HashMap<String, BoundFunctionDef>,
            pub fields: &'a HashMap<String, BoundFieldDef>,
        }

        let Self {
            name,
            size,
            type_id,
            is_refable,
            cloner: _,
            dropper: _,
            instance_functions,
            extension_functions,
            static_functions,
            fields,
        } = self;

        Debug::fmt(&BoundTypeDef {
            name,
            size,
            type_id,
            is_refable,
            instance_functions,
            extension_functions,
            static_functions,
            fields,
        }, f)
    }
}

impl BoundTypeDef {
    pub fn new(
        name: impl Into<String>,
        size: usize,
        type_id: impl Into<String>,
        is_refable: bool,
        cloner: Option<CloneProxy>,
        dropper: Option<DropProxy>,
    ) -> Self {
        assert_ne!(size, 0, "Bound types cannot be zero-sized");

        BoundTypeDef {
            name: name.into(),
            size,
            type_id: type_id.into(),
            is_refable,
            cloner,
            dropper,
            instance_functions: HashMap::new(),
            extension_functions: HashMap::new(),
            static_functions: HashMap::new(),
            fields: HashMap::new(),
        }
    }

    pub fn add_field(&mut self, field_def: BoundFieldDef)
        -> Result<(), BindingError> {
        if self.fields.contains_key(&field_def.name) {
            let qual_name = get_qualified_field_name(
                &field_def.name,
                &self.name,
            );
            return Err(BindingError {
                ty: BindingErrorType::DuplicateName,
                bound_name: qual_name,
                msg: "Field with same name is already bound".to_string(),
            });
        }

        self.fields.insert(field_def.name.clone(), field_def);

        Ok(())
    }
    
    pub fn add_instance_function(&mut self, fn_def: BoundFunctionDef)
        -> Result<(), BindingError> {
        if self.instance_functions.contains_key(&fn_def.name) {
            let qual_name = get_qualified_function_name(
                FunctionType::MemberInstance,
                &fn_def.name,
                Some(&self.name)
            );
            return Err(BindingError {
                ty: BindingErrorType::DuplicateName,
                bound_name: qual_name,
                msg: "Instance function with same name is already bound".to_string(),
            });
        }

        self.instance_functions.insert(fn_def.name.clone(), fn_def);

        Ok(())
    }

    pub fn add_static_function(&mut self, fn_def: BoundFunctionDef)
        -> Result<(), BindingError> {
        if self.static_functions.contains_key(&fn_def.name) {
            let qual_name = get_qualified_function_name(
                FunctionType::MemberStatic,
                &fn_def.name,
                Some(&self.name)
            );
            return Err(BindingError {
                ty: BindingErrorType::DuplicateName,
                bound_name: qual_name,
                msg: "Static function with same name is already bound".to_string(),
            });
        }

        self.static_functions.insert(fn_def.name.clone(), fn_def);

        Ok(())
    }

    pub fn add_extension_function(&mut self, fn_def: BoundFunctionDef)
        -> Result<(), BindingError> {
        if fn_def.param_types.is_empty() ||
            !(
                fn_def.param_types[0].ty == IntegralType::Object ||
                    fn_def.param_types[0].ty == IntegralType::Reference ||
                    fn_def.param_types[0].type_id
                        .as_ref()
                        .map(|id| id != &self.type_id)
                        .unwrap_or(false)
            ) {
            let qual_name = get_qualified_function_name(
                FunctionType::Extension,
                &fn_def.name,
                Some(&self.name)
            );
            return Err(BindingError {
                ty: BindingErrorType::InvalidDefinition,
                bound_name: qual_name,
                msg: "First parameter of extension function must match extended type".to_string(),
            });
        }

        if !self.extension_functions.contains_key(&fn_def.name) ||
            self.instance_functions.contains_key(&fn_def.name) {
            let qual_name = get_qualified_function_name(
                FunctionType::MemberStatic,
                &fn_def.name,
                Some(&self.name)
            );
            return Err(BindingError {
                ty: BindingErrorType::DuplicateName,
                bound_name: qual_name,
                msg: "Extension or instance function with same name is already bound".to_string(),
            });
        }

        self.extension_functions.insert(fn_def.name.clone(), fn_def);

        Ok(())
    }
}

#[derive(Clone, Debug)]
pub struct BoundFunctionDef {
    pub name: String,
    pub ty: FunctionType,
    pub is_const: bool,
    pub param_types: Vec<ObjectType>,
    pub return_type: ObjectType,
    pub proxy: ProxiedNativeFunction,
}

impl BoundFunctionDef {
    pub fn new(
        name: impl Into<String>,
        ty: FunctionType,
        is_const: bool,
        param_types: Vec<ObjectType>,
        return_type: ObjectType,
        proxy: ProxiedNativeFunction,
    ) -> Self {
        Self {
            name: name.into(),
            ty,
            is_const,
            param_types,
            return_type,
            proxy,
        }
    }
}

#[derive(Clone, Debug)]
pub struct BoundFieldDef {
    pub name: String,
    pub ty: ObjectType,
    //TODO: we probably only need one function for getting the value
    pub access_proxy: fn(&WrappedObject, &ObjectType) -> WrappedObject,
    pub assign_proxy: Option<fn(&mut WrappedObject, &WrappedObject)>,
}

impl BoundFieldDef {
    pub fn new(
        name: impl Into<String>,
        ty: ObjectType,
        access_proxy: fn(&WrappedObject, &ObjectType) -> WrappedObject,
        assign_proxy: Option<fn(&mut WrappedObject, &WrappedObject)>,
    ) -> Self {
        Self {
            name: name.into(),
            ty,
            access_proxy,
            assign_proxy,
        }
    }
    
    pub fn get_value(&self, instance: &mut WrappedObject) -> WrappedObject {
        (self.access_proxy)(instance, &instance.ty)
    }

    pub fn set_value(&self, instance: &mut WrappedObject, value: &WrappedObject) {
        self.assign_proxy.unwrap()(instance, value);
    }
}

#[derive(Clone, Debug)]
pub struct BoundEnumDef {
    pub name: String,
    pub width: usize,
    pub type_id: String,
    pub values: HashMap<String, i64>,
    pub all_ordinals: HashSet<i64>,
}

impl BoundEnumDef {
    pub fn new(
        name: impl Into<String>,
        width: usize,
        type_id: impl Into<String>
    ) -> Self {
        BoundEnumDef {
            name: name.into(),
            width,
            type_id: type_id.into(),
            values: HashMap::new(),
            all_ordinals: HashSet::new(),
        }
    }

    pub fn add_value(&mut self, name: impl AsRef<str>, value: i64) -> Result<(), BindingError> {
        if self.values.contains_key(name.as_ref()) {
            return Err(BindingError {
                ty: BindingErrorType::DuplicateName,
                bound_name: format!("{}::{}", self.name, name.as_ref()),
                msg: "Enum value with same name is already bound".to_string(),
            });
        }

        if self.all_ordinals.contains(&value) {
            return Err(BindingError {
                ty: BindingErrorType::Other,
                bound_name: format!("{}::{}", self.name, name.as_ref()),
                msg: "Enum value with same ordinal is already bound".to_string()
            });
        }

        self.values.insert(name.as_ref().to_string(), value);
        self.all_ordinals.insert(value);

        Ok(())
    }
}
