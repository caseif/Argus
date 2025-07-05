use std::any::Any;
use std::collections::HashMap;
use std::env;
use std::fs::File;
use std::io::{Cursor, Read};
use std::ops::Deref;
use std::path::{Path, PathBuf};
use std::sync::{Arc, LazyLock, RwLock};
use argus_logging::{debug, info, warn};
use arp::{load_arp_builtin_media_types, load_media_types_from_csv, Package, PackageSet, ResourceIdentifier};
use dashmap::DashMap;
use crate::*;
use crate::mappings::USER_MAPPINGS_CSV_CONTENT;

static MANAGER: LazyLock<ResourceManager> = LazyLock::new(ResourceManager::default);

const FILE_EXTENSION_ARP: &str = "arp";
const RESOURCES_DIR_NAME: &str = "resources";

#[derive(Default)]
pub struct ResourceManager {
    loaders: DashMap<String, Arc<Box<dyn ResourceLoader>>>,
    package_set: Arc<RwLock<PackageSet>>,
    fs_resources: DashMap<ResourceIdentifier, (PathBuf, String)>,
    loaded_resources: DashMap<ResourceIdentifier, WeakResource>,
}

impl ResourceManager {
    pub fn instance() -> &'static Self {
        MANAGER.deref()
    }

    pub fn add_memory_package(&self, buf: &'static [u8]) -> Result<(), String> {
        let package = Package::load_from_memory(buf)?;
        let mut set = self.package_set.write().unwrap();
        set.add_package(package);
        Ok(())
    }

    pub fn register_loader<T>(
        &self,
        media_types: Vec<&str>,
        loader: T,
    )
    where T: 'static + ResourceLoader {
        let arc: Arc<Box<dyn ResourceLoader>> = Arc::new(Box::new(loader));
        for media_type in media_types {
            self.loaders.insert(media_type.to_owned(), Arc::clone(&arc));
        }
    }

    pub fn get_resource(&self, uid: impl AsRef<str>) -> Result<Resource, ResourceError> {
        let uid_ref = uid.as_ref();
        let uid_parsed = parse_uid(uid_ref)?;

        if let Some(res) = self.loaded_resources.get(&uid_parsed).and_then(|weak| weak.upgrade()) {
            return Ok(res);
        };

        self.load_resource(&uid_parsed)
    }

    pub fn create_resource(
        &self,
        uid: &str,
        media_type: &str,
        data: Box<dyn Any + Send + Sync>,
    ) -> Result<Resource, ResourceError> {
        let uid_parsed = parse_uid(uid)?;

        if self.loaded_resources.contains_key(&uid_parsed) ||
            self.fs_resources.contains_key(&uid_parsed) ||
            self.package_set.read().unwrap().find_resource(&uid_parsed).is_ok() {
            return Err(ResourceError::new(
                ResourceErrorReason::AlreadyLoaded,
                uid,
                "Resource with the given UID is already present, cannot create"
            ));
        }

        let prototype = ResourcePrototype {
            uid: uid_parsed.clone(),
            media_type: media_type.to_owned(),
        };
        let resource = Resource::of(prototype.clone(), data);
        debug!(LOGGER, "Created new resource {} with media type {}", uid_parsed, media_type);
        self.loaded_resources.insert(uid_parsed, resource.downgrade());
        Ok(resource)
    }

    pub fn load_resource(&self, uid: &ResourceIdentifier) -> Result<Resource, ResourceError> {
        debug!(LOGGER, "Attempting to load resource with UID {}", uid);

        let (data, media_type) = if let Some(fs_ref) = self.fs_resources.get(uid)
            .filter(|fs_ref| fs_ref.value().0.is_file()) {
            debug!(LOGGER, "Attempting to load resource {} from bare filesystem", uid);
            
            let (path, media_type) = fs_ref.value();
            let data = self.load_fs_resource(uid, path)?;

            debug!(LOGGER, "Discovered resource {} on filesystem", uid);

            (data, media_type.clone())
        } else {
            debug!(LOGGER, "Attempting to load resource {} from ARP package", uid);

            let res = self.load_arp_resource(uid)?;

            debug!(LOGGER, "Discovered resource {} in ARP package", uid);

            res
        };

        debug!(LOGGER, "Trying to get loader for media type {}", media_type);

        let Some(loader) = self.loaders.get(&media_type) else {
            return Err(ResourceError::new(
                ResourceErrorReason::NoLoader,
                uid.to_string(),
                format!("No loader registered for media type {}", media_type)
            ));
        };

        let prototype = ResourcePrototype {
            uid: uid.clone(),
            media_type: media_type.clone(),
        };
        let size = data.len() as u64;
        let mut reader = Cursor::new(data);

        let resource_obj = loader.load_resource(self, &prototype, &mut reader, size)?;

        debug!(LOGGER, "Loaded resource {}", uid);

        Ok(Resource::of(prototype, resource_obj))
    }

    fn load_fs_resource(&self, uid: &ResourceIdentifier, path: &Path)
        -> Result<Vec<u8>, ResourceError> {
            File::open(path).and_then(|mut file| {
                let mut file_buf = Vec::new();
                file.read_to_end(&mut file_buf)?;
                Ok(file_buf)
            })
                .map_err(|err| ResourceError::new(
                    ResourceErrorReason::LoadFailed,
                    uid.to_string(),
                    err.to_string(),
                ))
    }

    fn load_arp_resource(&self, uid: &ResourceIdentifier)
        -> Result<(Vec<u8>, String), ResourceError> {
        let desc = self.package_set.read().unwrap().find_resource(uid)
            .map_err(|err| ResourceError::new(
                ResourceErrorReason::NotFound,
                uid.to_string(),
                err,
            ))?;
        Ok((
            desc.load()
                .map_err(|err|
                    ResourceError::new(ResourceErrorReason::LoadFailed, uid.to_string(), err))?,
            desc.media_type,
        ))
    }

    pub fn discover_resources(&self) {
        let cwd = env::current_dir().expect("Failed to get working directory");

        let resources_root_path = cwd.join(RESOURCES_DIR_NAME);
        if !resources_root_path.exists() {
            warn!(LOGGER, "Resources directory not found, resources will not be discovered!");
            return;
        }
        if !resources_root_path.is_dir() {
            warn!(
                LOGGER,
                "Resources directory path does not point to directory, \
                 resources will not be discovered!",
            );
            return;
        }

        debug!(LOGGER, "Discovering ARP packages");
        let package_count = self.discover_arp_resources(&resources_root_path);
        debug!(LOGGER, "Discovering loose resources from filesystem");
        self.discover_fs_resources(&resources_root_path);
        debug!(
            LOGGER,
            "Finished discovering resources (found {} ARP packages and {} loose resources)",
            package_count,
            self.fs_resources.len(),
        );
    }

    fn discover_arp_resources(&self, root_path: &Path) -> u64 {
        let root = root_path.read_dir().expect("Failed to enumerate resources root");

        let mut set = self.package_set.write().unwrap();
        let mut total = 0;

        for child_res in root {
            let child = match child_res {
                Ok(child) => child,
                Err(err) => {
                    warn!(LOGGER, "Failed to enumerate directory child: {}", err.to_string());
                    continue;
                }
            };
            let child_path = &child.path();

            let Some(ext) = child_path.extension() else { continue };
            if ext != FILE_EXTENSION_ARP {
               continue;
            }

            let package = match Package::load_from_file(child.path()) {
                Ok(package) => package,
                Err(err) => {
                    warn!(LOGGER, "Failed to load package at path {:?}: {}", child_path, err);
                    continue;
                }
            };
            set.add_package(package);
            info!(LOGGER, "Loaded resources from package at {:?}", child_path);

            total += 1;
        }

        total
    }

    fn discover_fs_resources(&self, root_path: &Path) {
        let root = root_path.read_dir().expect("Failed to enumerate resources root");

        let builtin_media_types = load_arp_builtin_media_types();
        let user_media_types = load_media_types_from_csv(USER_MAPPINGS_CSV_CONTENT);
        let all_media_types: HashMap<&'static str, &'static str> = builtin_media_types.into_iter()
            .chain(user_media_types)
            .collect();

        let mut dir_queue: Vec<(PathBuf, ResourceIdentifier)> = Vec::new();

        for root_child_res in root {
            let root_child = match root_child_res {
                Ok(de) => de,
                Err(err) => {
                    warn!(LOGGER, "Failed to enumerate directory child: {}", err.to_string());
                    continue;
                }
            };
            let root_child_path = root_child.path();

            if !root_child_path.is_dir() {
                warn!(LOGGER, "Ignoring non-directory '{:?}' in resources root", root_child_path);
                continue;
            }

            let dir_name_os = root_child_path.file_name()
                .expect("Failed to get file name").to_str();
            let Some(namespace) = dir_name_os else {
                warn!(
                    LOGGER,
                    "Directory name '{:?}' in resources root is not representable as UTF-8, \
                     ignoring",
                    dir_name_os,
                );
                continue;
            };

            dir_queue.push((
                root_child_path.clone(),
                ResourceIdentifier::new(namespace, vec![]),
            ));
        }

        while let Some((cur_path, cur_uid)) = dir_queue.pop() {
            let cur_dir = match cur_path.read_dir() {
                Ok(d) => d,
                Err(err) => {
                    warn!(
                        LOGGER,
                        "Failed to enumerate directory '{:?}', ignoring ({})",
                        cur_path,
                        err.to_string(),
                    );
                    continue;
                }
            };
            // since I keep accidentally writing this instead of child_path
            drop(cur_path);

            for child_res in cur_dir {
                let child = match child_res {
                    Ok(de) => de,
                    Err(err) => {
                        warn!(LOGGER, "Failed to enumerate directory child: {}", err.to_string());
                        continue;
                    }
                };
                let child_path = &child.path();

                if child_path.is_file() {
                    let Some(file_stem) = child_path.file_stem().unwrap().to_str() else {
                        warn!(
                            LOGGER,
                            "File name at '{:?}' in resources folder is not \
                             representable as UTF-8, ignoring",
                            child_path,
                        );
                        continue;
                    };
                    let new_uid = match cur_uid.join(file_stem) {
                        Ok(uid) => uid,
                        Err(err) => {
                            warn!(
                                LOGGER,
                                "File name '{:?}' cannot be converted to a resource \
                                 UID component, ignoring ({})",
                                file_stem,
                                err,
                            );
                            continue;
                        }
                    };
                    let Some(ext_os) = child_path.extension() else {
                        warn!(
                            LOGGER,
                            "Discovered filesystem resource {} without extension, ignoring",
                            cur_uid,
                        );
                        continue;
                    };
                    let Some(ext) = ext_os.to_str() else {
                        warn!(
                            LOGGER,
                            "File extension in {:?} is not representable as UTF-8, ignoring",
                            ext_os,
                        );
                        continue;
                    };
                    let Some(&media_type) = all_media_types.get(ext) else {
                        warn!(
                            LOGGER,
                            "Discovered filesystem resource {} with unknown extension {}, ignoring",
                            new_uid,
                            ext,
                        );
                        continue;
                    };
                    debug!(LOGGER, "Discovered filesystem resource {}", new_uid);
                    self.fs_resources.insert(new_uid, (child_path.clone(), media_type.to_owned()));
                } else if child_path.is_dir() {
                    let dir_name_os = child.file_name();
                    let Some(dir_name) = dir_name_os.to_str() else {
                        warn!(
                            LOGGER,
                            "Directory name at '{:?}' in resources folder is not \
                             representable as UTF-8, ignoring",
                            dir_name_os,
                        );
                        continue;
                    };
                    let new_uid = match cur_uid.join(dir_name) {
                        Ok(uid) => uid,
                        Err(err) => {
                            warn!(
                                LOGGER,
                                "Directory name '{}' cannot be converted to a resource \
                                 UID component, ignoring ({})",
                                dir_name,
                                err
                            );
                            continue;
                        }
                    };
                    dir_queue.push((child_path.clone(), new_uid));
                } else {
                    warn!(
                        LOGGER,
                        "Ignoring file at {:?} (not a regular file or directory)",
                        child_path,
                    );
                    continue;
                }
            }
        }
    }

    pub(crate) fn notify_unload(
        &self,
        prototype: &ResourcePrototype,
    ) {
        // remove it from the set of loaded resources so the allocation can be dropped
        self.loaded_resources.remove(&prototype.uid);
        debug!(LOGGER, "Resource {} was unloaded", prototype.uid);
    }
}

fn parse_uid(uid: impl AsRef<str>) -> Result<ResourceIdentifier, ResourceError> {
    let uid_ref = uid.as_ref();
    ResourceIdentifier::parse(uid_ref)
        .map_err(|err| ResourceError::new(ResourceErrorReason::InvalidContent, uid_ref, err))
}
