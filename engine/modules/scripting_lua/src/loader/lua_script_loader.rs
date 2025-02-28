use std::any::Any;
use std::io::Read;
use argus_resman::{ResourceError, ResourceErrorReason, ResourceLoader, ResourceManager, ResourcePrototype};
use crate::loaded_script::LoadedScript;

pub(crate) struct LuaScriptLoader {
}

impl ResourceLoader for LuaScriptLoader {
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

        let script_src = match String::from_utf8(data) {
            Ok(text) => text,
            Err(_) => {
                return Err(ResourceError::new(
                    ResourceErrorReason::MalformedContent,
                    prototype.uid.to_string(),
                    "Lua script is not valid UTF-8",
                ))
            }
        };

        let script = LoadedScript::new(script_src);
        let res = Box::new(script);
        Ok(res)
    }
}
