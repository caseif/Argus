use std::any::Any;
use std::io::Read;
use resman_rs::*;
use serde::Deserialize;
use serde_json::error::Category;
use crate::common::Material;

pub(crate) struct MaterialLoader {
}

impl ResourceLoader for MaterialLoader {
    fn load_resource(
        &self,
        _manager: &ResourceManager,
        prototype: &ResourcePrototype,
        reader: &mut dyn Read,
        _size: u64
    ) -> Result<Box<dyn Any + Send + Sync>, ResourceError> {
        const BUF_LEN: usize = 1024;
        let mut buf = [0u8; BUF_LEN];
        let mut data: Vec<u8> = Vec::with_capacity(BUF_LEN);
        loop {
            let read_bytes = reader.read(buf.as_mut_slice())
                .map_err(|err| ResourceError::new(
                    ResourceErrorReason::LoadFailed,
                    prototype.uid.to_string(),
                    err.to_string()
                ))?;
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
                    prototype.uid.to_string(),
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
                    prototype.uid.to_string(),
                    "Sprite definition structure is invalid"
                ));
            }
        };
        let mut dependencies = Vec::with_capacity(material.shader_uids.len() + 1);
        dependencies.push(material.texture_uid.clone());
        dependencies.extend(material.shader_uids.clone());
        //handle.load_dependencies(&mut manager, dependencies)?;

        Ok(Box::new(material))
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
