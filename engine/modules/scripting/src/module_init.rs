use crate::config::ScriptingConfig;
use crate::register::register_script_bindings;
use crate::*;
use argus_core::{register_module, run_on_update_thread, EngineManager, LifecycleStage, Ordering};
use argus_logging::debug;

const CONFIG_KEY_SCRIPTING: &str = "scripting";

const INIT_FN_NAME: &str = "init";

#[register_module(id = "scripting", depends(core))]
pub fn update_lifecycle_scripting(stage: LifecycleStage) {
    match stage {
        LifecycleStage::PreInit => {
            EngineManager::instance()
                .add_config_deserializer::<ScriptingConfig>(CONFIG_KEY_SCRIPTING);
        }
        LifecycleStage::Init => {
            debug!(LOGGER, "Initializing scripting");
            register_script_bindings();
        }
        LifecycleStage::PostInit => {
            // parameter type resolution is deferred to ensure that all
            // types have been registered first
            ScriptManager::instance().get_bindings().resolve_types()
                .expect("Failed to resolve parameter types");

            ScriptManager::instance().apply_bindings_to_all_contexts()
                .expect("Failed to apply bindings to script contexts");

            let config = EngineManager::instance().get_config();
            let main = config.get_section::<ScriptingConfig>()
                .and_then(|sec| sec.main.as_ref())
                .cloned();
            if let Some(init_script_uid) = main.clone() {
                // run it during the first iteration of the update loop
                run_on_update_thread(
                    Box::new(move || { run_init_script(init_script_uid.clone()); }),
                    Ordering::Standard,
                );
            }
        }
        LifecycleStage::Deinit => {
            ScriptManager::instance().perform_deinit();
        }
        _ => {}
    }
}

fn run_init_script(uid: impl AsRef<str>) {
    let context_handle = match load_script_with_new_context(uid) {
        Ok(handle) => handle,
        Err(err) => {
            panic!("Failed to run init script: {}", err.msg);
        }
    };

    let mgr = ScriptManager::instance();
    let mut context = mgr.get_context_mut(context_handle);
    let init_res = context.value_mut().get_mut().invoke_script_function(INIT_FN_NAME, Vec::new());
    if let Err(err) = init_res {
        panic!("Failed to run init script: {}", err.message);
    }
}
