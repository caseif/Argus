use crate::bind::{BoundEnumDef, BoundFunctionDef, BoundTypeDef};
use crate::context::ScriptContext;
use crate::error::ScriptLoadError;
use argus_scripting_bind::{ScriptInvocationError, WrappedObject};
use std::any::Any;
use std::rc::Rc;
use parking_lot::ReentrantMutex;
use argus_resman::Resource;

pub trait ScriptLanguagePlugin: Send + Sync {
    fn get_language_name() -> &'static str
    where
        Self: Sized;

    fn get_media_types() -> &'static [&'static str]
    where
        Self: Sized;

    fn create_context_data(&mut self) -> Rc<ReentrantMutex<dyn Any>>;

    fn load_script(
        &mut self,
        context: &mut ScriptContext,
        resource: Resource,
    ) -> Result<(), ScriptLoadError>;

    fn bind_type(&mut self, context: &mut ScriptContext, type_def: &BoundTypeDef);

    fn bind_global_function(&mut self, context: &mut ScriptContext, fn_def: &BoundFunctionDef);

    fn bind_enum(&mut self, context: &mut ScriptContext, enum_def: &BoundEnumDef);

    fn commit_bindings(&mut self, context: &mut ScriptContext);

    fn invoke_script_function(
        &mut self,
        context: &mut ScriptContext,
        name: &str,
        params: Vec<WrappedObject>,
    ) -> Result<WrappedObject, ScriptInvocationError>;
}
