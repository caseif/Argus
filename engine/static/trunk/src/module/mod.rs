use std::collections::HashMap;
use std::sync::Mutex;
use std::path::PathBuf;

use lazy_static::lazy_static;

type LifecycleFn = fn(LifecycleStage);

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

pub fn register_module(mod_name: &str, lifecycle_fn: LifecycleFn) {
    g_module_registrations.lock().unwrap().insert(mod_name.to_string(), lifecycle_fn);
}

pub fn register_dynamic_module(id: String, lifecycle_callback: LifecycleFn, dependencies: Vec<String>) {
    //TODO
}

pub(crate) fn get_present_dynamic_modules() -> HashMap<String, PathBuf> {
    return HashMap::new(); //TODO
}

pub(crate) fn enable_dynamic_module(module_id: String) -> bool{
    return false; //TODO
}

pub(crate) fn enable_modules(modules: Vec<String>) {
    //TODO
}

pub(crate) fn unload_dynamic_modules() {
    //TODO
}

pub(crate) fn init_modules() {
    //TODO
}

pub(crate) fn deinit_modules() {
    //TODO
}
