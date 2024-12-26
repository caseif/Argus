use std::ops::Deref;
use resman_rustabi::argus::resman::{ResourceError, ResourceErrorReason, ResourceLoader, ResourceManager, ResourcePrototype, WrappedResourceLoader};
use serde::Deserialize;
use serde_json::error::Category;
use crate::common::Material;

pub(crate) struct MaterialLoader {
}

impl ResourceLoader for MaterialLoader {
    fn load_resource(
        &mut self,
        mut handle: WrappedResourceLoader,
        mut manager: ResourceManager,
        prototype: ResourcePrototype,
        read_callback: Box<dyn Fn(&mut [u8], usize) -> usize>,
        _size: usize
    ) -> Result<*mut u8, ResourceError> {
        const BUF_LEN: usize = 1024;
        let mut buf = [0u8; BUF_LEN];
        let mut data: Vec<u8> = Vec::with_capacity(BUF_LEN);
        loop {
            let read_bytes = read_callback.deref()(buf.as_mut_slice(), BUF_LEN);
            if read_bytes == 0 {
                break;
            }

            data.append(&mut buf[0..read_bytes].to_vec());
        }

        let material_json = match String::from_utf8(data) {
            Ok(json) => json,
            Err(_) => {
                return Err(ResourceError::new(
                    ResourceErrorReason::MalformedContent,
                    prototype.uid.as_str(),
                    "Sprite definition is not valid UTF-8",
                ))
            }
        };

        let material = match parse_material(material_json) {
            Ok(m) => m,
            Err(e) => {
                let reason = match e.classify() {
                    Category::Io => ResourceErrorReason::LoadFailed,
                    Category::Syntax => ResourceErrorReason::MalformedContent,
                    Category::Data => ResourceErrorReason::InvalidContent,
                    Category::Eof => ResourceErrorReason::MalformedContent,
                };
                return Err(ResourceError::new(
                    reason,
                    prototype.uid.as_str(),
                    "Sprite definition structure is invalid"
                ));
            }
        };
        let mut dependencies = Vec::with_capacity(material.shader_uids.len() + 1);
        dependencies.push(material.texture_uid.clone());
        dependencies.extend(material.shader_uids.clone());
        handle.load_dependencies(&mut manager, dependencies)?;

        Ok(Box::into_raw(Box::new(material)).cast())
    }

    fn copy_resource(
        &mut self,
        _handle: WrappedResourceLoader,
        _manager: ResourceManager,
        _prototype: ResourcePrototype,
        src_data: *const u8
    ) -> Result<*mut u8, ResourceError> {
        let material: &Material = unsafe { src_data.cast::<Material>().as_ref().unwrap() };
        Ok(Box::into_raw(Box::new(material.clone())).cast())
    }

    fn unload_resource(&mut self, _handle: WrappedResourceLoader, ptr: *mut u8) {
        unsafe {
            _ = Box::from_raw(ptr.cast::<Material>());
        }
    }
}

#[derive(Deserialize)]
struct MaterialResourceModel {
    texture: String,
    shaders: Vec<MaterialResourceShaderModel>,
}

#[derive(Deserialize)]
struct MaterialResourceShaderModel {
    stage: String,
    uid: String,
}

fn parse_material(json_str: String) -> Result<Material, serde_json::Error> {
    let raw_model = serde_json::from_str::<MaterialResourceModel>(json_str.as_str())?;
    Ok(Material::new(raw_model.texture, raw_model.shaders.iter().map(|s| s.uid.clone()).collect()))
}
