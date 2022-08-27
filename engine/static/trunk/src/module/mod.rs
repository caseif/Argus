use std::collections::{HashMap, HashSet};
use std::sync::Mutex;
use std::path::PathBuf;

use lazy_static::lazy_static;

use crate::engine::EngineHandle;

pub type LifecycleFn = fn(Box<EngineHandle>, LifecycleStage);

lazy_static! {
    static ref g_module_registrations: Mutex<HashMap<String, LifecycleFn>> = Mutex::new(HashMap::new());
}

/**
 * @brief Represents the stages of engine bring-up or spin-down.
 */
pub enum LifecycleStage {
    /**
     * @brief The very first lifecycle stage, intended to be used for tasks
     *        such as shared library loading which need to occur before any
     *        "real" lifecycle stages are loaded.
     */
    Load,
    /**
     * @brief Early initialization stage for performing initialization
     *        which other modules may be contingent on.
     *
     * Should be used for performing early allocation or other early setup,
     * generally for the purpose of preparing the module for use in the
     * initialization of dependent modules.
     */
    PreInit,
    /**
     * @brief Primary initialization stage for performing most
     *        initialization tasks.
     */
    Init,
    /**
     * @brief Post-initialization stage for performing initialization
     *        contingent on all parent modules being initialized.
     */
    PostInit,
    /**
     * @brief Early de-initialization. This occurs directly after the engine
     *        has committed to shutting down and has halted update callbacks
     *        on all primary threads.
     *
     * Should be used for performing early de-initialization tasks, such as
     * saving user data. Changes during this stage should not be visible to
     * dependent modules.
     */
    PreDeinit,
    /**
     * @brief Primary de-initialization.
     *
     * Should be used for performing most de-initialization tasks.
     */
    Deinit,
    /**
     * @brief Very late de-initialization.
     *
     * Should be used for performing de-init contingent on parent modules
     * being fully de-initialized as well as for final deallocation and
     * similar tasks.
     */
    PostDeinit
}

pub(crate) struct StaticModule {
    id: String,
    dependencies: HashSet<String>,
    lifecycle_update_callback: LifecycleFn,
}

/**
 * @brief Represents a module to be dynamically loaded by the Argus engine.
 *
 * This struct contains all information required to initialize and update
 * the module appropriately.
 */
pub struct DynamicModule {
    /**
     * @brief The ID of the module.
     *
     * @attention This ID must contain only lowercase Latin letters
     *            (`[a-z]`), numbers (`[0-9]`), and underscores (`[_]`).
     */
    id: String,

    /**
     * @brief The function which handles lifecycle updates for this module.
     *
     * This function will accept a single argument of type `const`
     * LifecycleStage and will not return anything.
     *
     * This function should handle initialization of the module when the
     * engine starts, as well as deinitialization when the engine stops.
     *
     * @sa LifecycleStage
     */
    lifecycle_update_callback: LifecycleFn,

    /**
     * @brief A list of IDs of modules this one is dependent on.
     *
     * If any dependency fails to load, the dependent module will also fail.
     */
    dependencies: HashSet<String>,

    /**
     * @brief An opaque handle to the shared library containing the module.
     *
     * @warning This is intended for internal use only.
     */
    handle: usize //TODO
}

pub fn register_module(mod_name: &str, lifecycle_fn: LifecycleFn) {
    g_module_registrations.lock().unwrap().insert(mod_name.to_string(), lifecycle_fn);
}

pub fn register_dynamic_module(id: &str, lifecycle_callback: LifecycleFn, dependencies: Vec<String>) {
    //TODO
}

pub(crate) fn get_present_dynamic_modules() -> HashMap<String, PathBuf> {
    return HashMap::new(); //TODO
}

impl EngineHandle {
    pub(crate) fn enable_dynamic_module(&mut self, module_id: String) -> bool{
        return false; //TODO
    }

    pub(crate) fn enable_modules(&mut self, modules: Vec<String>) {
        //TODO
    }

    pub(crate) fn unload_dynamic_modules(&mut self) {
        //TODO
    }

    pub(crate) fn init_modules(&mut self) {
        //TODO
    }

    pub(crate) fn deinit_modules(&mut self) {
        //TODO
    }
}