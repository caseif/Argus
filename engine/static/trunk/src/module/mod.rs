use std::collections::{HashMap, HashSet, VecDeque};
use std::ops::RangeBounds;
use std::{env, fmt, error};
use std::env::consts::{DLL_PREFIX, DLL_SUFFIX};
use std::fs;
use std::sync::Mutex;
use std::path::PathBuf;

use crate::LOGGER;
use crate::STATIC_MODULE_DEFS;
use crate::engine::EngineHandle;

use lazy_static::lazy_static;
use lowlevel::logging::Logger;

pub type LifecycleFn = fn(&EngineHandle, LifecycleStage);

const MODULES_DIR_NAME: &str = "modules";

lazy_static! {
    static ref g_module_registrations: Mutex<HashMap<String, LifecycleFn>> = Mutex::new(HashMap::new());
    static ref g_dyn_module_registrations: Mutex<HashMap<String, DynamicModule>> = Mutex::new(HashMap::new());
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct CircularDependencyError {
    message: String
}

impl CircularDependencyError {
    pub fn new(message: &str) -> Self {
        CircularDependencyError { message: message.to_string() }
    }
}

impl error::Error for CircularDependencyError {}

impl fmt::Display for CircularDependencyError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "{}", self.message)
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ModuleRegistrationError {
    message: String
}

impl ModuleRegistrationError {
    pub fn new(message: &str) -> Self {
        ModuleRegistrationError { message: message.to_string() }
    }
}

impl error::Error for ModuleRegistrationError {}

impl fmt::Display for ModuleRegistrationError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "{}", self.message)
    }
}

/**
 * @brief Represents the stages of engine bring-up or spin-down.
 */
#[derive(Clone, Copy, Debug, PartialEq, PartialOrd)]
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

static INIT_LIFECYCLE_STAGES: [LifecycleStage; 3] = [LifecycleStage::PreInit,
    LifecycleStage::Init, LifecycleStage::PostInit];

static DEINIT_LIFECYCLE_STAGES: [LifecycleStage; 3] = [LifecycleStage::PreDeinit, LifecycleStage::Deinit,
    LifecycleStage::PostDeinit];

pub(crate) struct StaticModule {
    id: String,
    dependencies: HashSet<&'static str>,
    lifecycle_update_callback: LifecycleFn,
}

/**
 * @brief Represents a module to be dynamically loaded by the Argus engine.
 *
 * This struct contains all information required to initialize and update
 * the module appropriately.
 */
#[derive(Clone)]
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
    dependencies: HashSet<String>
}

pub fn register_module(mod_id: &str, lifecycle_fn: LifecycleFn) {
    LOGGER.debug(format!("Registering static module {}", mod_id));
    g_module_registrations.lock().unwrap().insert(mod_id.to_string(), lifecycle_fn);
}

pub fn register_dynamic_module(id: &'static str, lifecycle_callback: LifecycleFn, dependencies: &[&str])
        -> Result<(), ModuleRegistrationError> {
    if !id.chars().all(|c| c == '_' || c.is_numeric() || c.is_lowercase()) {
        return Err(ModuleRegistrationError::new("Module identifier must contain only lowercase letters"));
    }

    if STATIC_MODULE_DEFS.contains_key(id) {
        return Err(ModuleRegistrationError::new(format!(
            "Module identifier is already in use by static module: {}", id).as_str()));
    }

    if g_dyn_module_registrations.lock().unwrap().contains_key(id) {
        return Err(ModuleRegistrationError::new(format!("Module is already registered: {}", id).as_str()));
    }

    let module = DynamicModule {
        id: id.to_string(),
        lifecycle_update_callback: lifecycle_callback,
        dependencies: HashSet::from_iter(dependencies.iter().map(|d| d.to_string()))
    };

    g_dyn_module_registrations.lock().unwrap().insert(id.to_string(), module);

    LOGGER.debug(format!("Registered dynamic module {}", id));

    return Ok(());
}

pub(crate) fn get_present_dynamic_modules() -> HashMap<String, PathBuf> {
    let cwd = match env::current_dir() {
        Ok(p) => p,
        Err(e) => {
            LOGGER.warn(format!("Failed to get working directory: {:?}", e));
            return HashMap::new();
        }
    };
    
    let modules_dir_path = cwd.join(MODULES_DIR_NAME);

    if !modules_dir_path.is_dir() {
        LOGGER.info("No dynamic modules to load");
        return HashMap::new();
    }

    let mut modules = HashMap::<String, PathBuf>::new();

    let paths = match fs::read_dir(modules_dir_path) {
        Ok(p) => p,
        Err(e) => {
            LOGGER.warn(format!("Failed to open modules directory: {:?}", e));
            return HashMap::new();
        }
    };

    for entry_res in paths {
        let entry = match entry_res {
            Ok(p) => p,
            Err(e) => {
                continue;
            }
        }.path();

        let file_name = match entry.file_name() {
            Some(s) => s,
            None => {
                return HashMap::new();
            }
        };

        let file_stem = match entry.file_stem() {
            Some(s) => s,
            None => {
                return HashMap::new();
            }
        };

        if !entry.is_dir() {
            LOGGER.debug(format!("Ignoring non-regular module file {}", file_name.to_string_lossy()));
            continue;
        }

        if DLL_PREFIX.len() > 0 && !file_stem.to_string_lossy().starts_with(DLL_PREFIX) {
            LOGGER.debug(format!("Ignoring module file {} with non-library prefix", file_name.to_string_lossy()));
            continue;
        }

        if entry.extension().map_or("", |s| s.to_str().unwrap_or("")) != DLL_SUFFIX {
            LOGGER.warn(format!("Ignoring module file {} with non-library extension", file_name.to_string_lossy()));
            continue;
        }

        let base_name = &file_stem.to_string_lossy()[DLL_PREFIX.len()..];
        modules.insert(base_name.to_string(), entry);
    }

    if modules.is_empty() {
        LOGGER.info("No dynamic modules present");
    }

    return modules;
}

fn locate_dynamic_module(id: &String) -> Option<PathBuf> {
    /*auto modules_dir_path = std::filesystem::current_path() / MODULES_DIR_NAME;

    if (!std::filesystem::is_directory(modules_dir_path)) {
        Logger::default_logger().warn("Dynamic module directory not found. (Searched at %s)", modules_dir_path.c_str());
        return "";
    }

    auto module_path = modules_dir_path / (SHARED_LIB_PREFIX + id + EXTENSION_SEPARATOR SHARED_LIB_EXT);
    if (!std::filesystem::is_regular_file(module_path)) {
        Logger::default_logger().warn("Item referred to by %s does not exist, is not a regular file, or is inaccessible",
                module_path.c_str());
        return "";
    }

    return module_path;*/
    return Some("".into());
}

fn topo_sort<T: Clone + PartialEq>(nodes: Vec<T>, edges: Vec<(&T, &T)>) -> Result<Vec<T>, CircularDependencyError> {
    let mut sorted_nodes = Vec::<T>::new();
    let mut start_nodes: VecDeque<&T> = nodes.iter().collect();
    let mut remaining_edges = edges;

    for edge in &remaining_edges {
        start_nodes.retain(|n| *n != edge.1);
    }

    while !start_nodes.is_empty() {
        let cur_node = start_nodes.pop_front().unwrap();
        if !sorted_nodes.contains(&cur_node) {
            sorted_nodes.push(cur_node.clone());
        }

        let mut remove_edges = Vec::<(&T, &T)>::new();
        for cur_edge in &remaining_edges {
            let dest_node = &cur_edge.1;
            if cur_edge.0 != cur_node {
                continue;
            }

            remove_edges.push(cur_edge.clone());

            let mut has_incoming_edges = false;
            for check_edge in remaining_edges.iter() {
                if check_edge != cur_edge && &check_edge.1 == dest_node {
                    has_incoming_edges = true;
                    break;
                }
            }

            if !has_incoming_edges {
                start_nodes.push_back(dest_node.clone());
            }
        }

        for edge in remove_edges {
            remaining_edges.retain(|e| e != &edge);
        }
    }

    if !remaining_edges.is_empty() {
        return Err(CircularDependencyError::new("Graph contains cycles"));
    }

    return Ok(sorted_nodes);
}

fn topo_sort_modules(module_map: HashMap<String, DynamicModule>)
        -> Result<Vec<DynamicModule>, CircularDependencyError> {
    let mut module_ids = Vec::<String>::new();
    let mut edges = Vec::<(&String, &String)>::new();

    for (mod_id, module) in &module_map {
        module_ids.push(mod_id.clone());
        for dep in &module.dependencies {
            if module_map.contains_key(dep) {
                edges.push((dep, mod_id));
            }
        }
    }

    let mut sorted_modules = Vec::<DynamicModule>::new();
    
    match topo_sort(module_ids, edges) {
        Ok(sorted_ids) => {
            for id in sorted_ids {
                sorted_modules.push(module_map[&id].clone());
            }
        },    
        Err(_) => {
            LOGGER.fatal("Circular dependency detected in dynamic modules, cannot proceed.");
        }
    }

    return Ok(sorted_modules);
}

fn log_dependent_chain(logger: &Logger, dependent_chain: &Vec<String>, warn: bool) {
    for dependent in dependent_chain {
        let formatted = format!("    Required by module \"{}\"", dependent);
        if warn {
            logger.warn(formatted);
        } else {
            logger.debug(formatted);
        }
    }
}

impl EngineHandle {
    fn load_dynamic_module<'a>(&'a mut self, id: &String, dependent_chain: Vec<String>) -> Option<&'a DynamicModule> {
        let path = match locate_dynamic_module(id) {
            Some(p) => p,
            None => {
                LOGGER.warn(format!("Dynamic module {} was requested but could not be located", id.as_str()));
                log_dependent_chain(&LOGGER, &dependent_chain, true);
                return None;
            }
        };

        LOGGER.debug(format!("Attempting to load dynamic module {} from file {}", id.as_str(), path.to_string_lossy()));
        log_dependent_chain(&LOGGER, &dependent_chain, false);

        let lib: libloading::Library;
        unsafe {
            lib = match libloading::Library::new(path) {
                Ok(l) => l,
                Err(e) => {
                    LOGGER.warn(format!("Failed to load dynamic module {} (error: {})", id, e));
                    log_dependent_chain(&LOGGER, &dependent_chain, true);
                    return None;
                }
            };
        }

        let module = match self.module_state.dyn_module_registrations.get_mut(id) {
            Some(m) => m,
            None => {
                LOGGER.warn(format!(
                    "Module {} attempted to register itself by a different ID than indicated by its filename",
                    id
                ));
                log_dependent_chain(&LOGGER, &dependent_chain, true);
                return None;
            }
        };

        self.module_state.dyn_library_handles.insert(id.to_owned(), lib);

        return Some(module);
    }

    fn enable_dynamic_module_with_dependent_chain(&mut self, module_id: &String, dependent_chain: &Vec<String>) -> bool {
        if self.module_state.enabled_dyn_modules_staging.contains_key(module_id) {
            LOGGER.info(format!("Dynamic module \"{}\" was requested while already enabled",
                &module_id));
            return true;
        }

        // skip duplicates
        for (enabled_mod_id, enabled_mod) in &self.module_state.enabled_dyn_modules_staging {
            if enabled_mod_id == module_id {
                if dependent_chain.is_empty() {
                    LOGGER.warn(format!("Module \"{}\" requested more than once.", module_id));
                }
                return false;
            }
        }

        let reg = match self.module_state.dyn_module_registrations.get(module_id) {
            Some(r) => r.clone(),
            None => {
                let loaded_module = match self.load_dynamic_module(module_id, vec![]) {
                    Some(m) => m,
                    None => {
                        return false;
                    }
                };

                match self.module_state.dyn_module_registrations.get(module_id) {
                    Some(r) => r.clone(),
                    None => {
                        LOGGER.warn(format!(
                            "Module \"{}\" was loaded but a matching registration was not found (name mismatch?)",
                            &module_id
                        ));

                        for dependent in dependent_chain {
                            LOGGER.warn(format!("    Required by module \"{}\"", dependent));
                        }
                        return false;
                    }
                }
            }
        };

        let mut new_chain = dependent_chain.clone();
        new_chain.push(module_id.to_owned());
        for dep in &reg.dependencies {
            // skip static modules since they're always loaded
            if STATIC_MODULE_DEFS.contains_key(dep.as_str()) {
                continue;
            }

            if !self.enable_dynamic_module_with_dependent_chain(dep, &new_chain) {
                //TODO: unload modules that were loaded to satisfy the dependency chain
                return false;
            }
        }

        self.module_state.enabled_dyn_modules_staging.insert(module_id.to_owned(), reg.clone());

        LOGGER.info(format!("Enabled dynamic module {}.", module_id));

        return true;
    }

    pub(crate) fn enable_dynamic_module(&mut self, module_id: &String) -> bool {
        self.enable_dynamic_module_with_dependent_chain(module_id, &vec![])
    }

    pub(crate) fn enable_modules(&mut self, modules: Vec<String>) {
        let mut all_modules = HashSet::<String>::new(); // requested + transitive modules

        for module_id in modules {
            match STATIC_MODULE_DEFS.get(module_id.as_str()) {
                Some(module_deps) => {
                    all_modules.insert(module_id);
                    for dep in module_deps {
                        all_modules.insert(dep.to_string());
                    }
                },
                None => {
                    self.enable_dynamic_module(&module_id);
                }
            }
        }

        // we add them to the master list like this in order to preserve the hardcoded load order
        for (module_id, module_deps) in STATIC_MODULE_DEFS.iter() {
            if all_modules.contains(&module_id.to_string()) {
                self.module_state.enabled_static_modules.push(StaticModule {
                    id: module_id.to_string(),
                    dependencies: HashSet::from_iter(module_deps.iter().cloned()),
                    lifecycle_update_callback: *(*g_module_registrations.lock().unwrap())
                        .get(&module_id.to_string()).unwrap()
                });
            }
        }

        // we'll sort the dynamic modules just before bringing them up, since
        // they can still be requested during the Load lifecycle event
    }

    pub(crate) fn unload_dynamic_modules(&mut self) {
        for (mod_id, mod_reg) in &self.module_state.dyn_module_registrations {
            self.module_state.enabled_dyn_modules.retain(|m| m.id != mod_reg.id);

            // this should unload the library automatically via the Drop impl
            self.module_state.dyn_library_handles.remove_entry(mod_id);
        }
    }

    pub(crate) fn init_modules(&mut self) {
        let dyn_mod_initial_count = self.module_state.enabled_dyn_modules_staging.len();
        self.module_state.enabled_dyn_modules
            = topo_sort_modules(self.module_state.enabled_dyn_modules_staging.clone()).unwrap();

        LOGGER.debug("Propagating Load lifecycle stage");
        // give modules a chance to request additional dynamic modules
        self.send_lifecycle_update(LifecycleStage::Load);
        // re-sort the dynamic module if it was augmented
        if self.module_state.enabled_dyn_modules_staging.len() > dyn_mod_initial_count {
            LOGGER.debug("Dynamic module list changed, must re-sort");
            self.module_state.enabled_dyn_modules
                = topo_sort_modules(self.module_state.enabled_dyn_modules_staging.clone()).unwrap();
        }

        self.module_state.enabled_dyn_modules_staging.clear();

        LOGGER.debug("Propagating remaining bring-up lifecycle stages");

        for stage in INIT_LIFECYCLE_STAGES {
            self.send_lifecycle_update(stage);
        }
    }

    pub(crate) fn deinit_modules(&self) {
        for stage in DEINIT_LIFECYCLE_STAGES {
            for module in &self.module_state.enabled_static_modules {
                (module.lifecycle_update_callback)(self, stage);
            }

            for module in &self.module_state.enabled_dyn_modules {
                (module.lifecycle_update_callback)(self, stage);
            }
        }
    }

    fn send_lifecycle_update(&mut self, stage: LifecycleStage) {
        for module in &self.module_state.enabled_static_modules {
            LOGGER.debug(format!("Sent {:?} lifecycle stage to module {}",
                    stage, module.id));
            (module.lifecycle_update_callback)(self, stage);
            LOGGER.debug(format!("Lifecycle stage {:?} was completed by module {}",
                    stage, module.id));
        }

        for module in &self.module_state.enabled_dyn_modules {
            LOGGER.debug(format!("Sent {:?} lifecycle stage to module {}",
                    stage, module.id));
            (module.lifecycle_update_callback)(self, stage);
            LOGGER.debug(format!("Lifecycle stage {:?} was completed by module {}",
                    stage, module.id));
        }

        self.prev_stage = Some(stage);
    }
}
