use std::any::Any;
use std::rc::Rc;
use argus_scripting_bind::{ScriptInvocationError, WrappedObject};
use parking_lot::{ReentrantMutex, ReentrantMutexGuard};
use resman_rs::{Resource, ResourceManager};
use crate::error::ScriptLoadError;
use crate::manager::ScriptManager;

pub type ScriptContextHandle = u64;

#[derive(Debug)]
pub struct ScriptContext {
    pub(crate) language: String,
    pub(crate) plugin_data: Option<Rc<ReentrantMutex<dyn Any>>>,
}

impl ScriptContext {
    pub fn get_language(&self) -> &str {
        self.language.as_str()
    }

    pub fn get_plugin_data(&self) -> ReentrantMutexGuard<dyn Any> {
        self.plugin_data.as_ref().expect("Language plugin data is missing").lock()
    }

    pub fn load_script_by_uid(&mut self, uid: impl AsRef<str>) -> Result<(), ScriptLoadError> {
        ScriptManager::instance().load_resource(&self.language, uid)
            .and_then(|res| self.load_script(res))
    }

    pub fn load_script(&mut self, resource: Resource) -> Result<(), ScriptLoadError> {
        let mgr = ScriptManager::instance();

        if mgr.get_media_type_language(&resource.get_prototype().media_type).as_ref() !=
            Some(&self.language) {
            return Err(ScriptLoadError::new(
                &resource.get_prototype().uid.to_string(),
                format!(
                    "Resource with media type '{}' cannot be loaded by plugin '{}'",
                    resource.get_prototype().media_type,
                    self.language,
                )));
        }

        mgr.move_resource(&self.language, resource.clone());
        let mut plugin = mgr.get_language_plugin_mut(&self.language).unwrap();
        plugin.load_script(self, resource)
    }

    pub fn invoke_script_function(
        &mut self,
        fn_name: impl AsRef<str>,
        params: Vec<WrappedObject>,
    ) -> Result<WrappedObject, ScriptInvocationError> {
        let mgr = ScriptManager::instance();
        let mut plugin = mgr.get_language_plugin_mut(&self.language)
            .expect("No plugin is loaded for context's associated language");
        plugin.invoke_script_function(self, fn_name.as_ref(), params)
    }
}

pub fn create_script_context(language: impl Into<String>) -> ScriptContext {
    let mgr = ScriptManager::instance();

    let lang_owned = language.into();

    let mut plugin = mgr.get_language_plugin_mut(&lang_owned)
        .expect("No plugin is loaded for context's associated language");

    let plugin_data = plugin.create_context_data();

    ScriptContext {
        language: lang_owned,
        plugin_data: Some(plugin_data),
    }
}

pub fn load_script_with_new_context(uid: impl AsRef<str>)
    -> Result<ScriptContextHandle, ScriptLoadError> {
    let resource = ResourceManager::instance().get_resource(uid.as_ref())
        .map_err(|err| {
            ScriptLoadError::new(
                uid.as_ref(),
                format!(
                    "Resource load failed: {:?}",
                    err,
                )
            )
        })?;

    let media_type = &resource.get_prototype().media_type;
    let Some(lang) = ScriptManager::instance().get_media_type_language(media_type)
        .map(|s| s.to_owned())
    else {
        return Err(ScriptLoadError::new(
            uid.as_ref(),
            format!(
                "No language registered for media type '{}'",
                resource.get_prototype().media_type,
            ),
        ));
    };

    let mut context = create_script_context(lang);

    context.load_script(resource)?;

    Ok(ScriptManager::instance().register_context(context))
}
