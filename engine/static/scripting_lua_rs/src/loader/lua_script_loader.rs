use std::ops::Deref;
use resman_rustabi::argus::resman::*;
use crate::loaded_script::LoadedScript;

pub(crate) struct LuaScriptLoader {
}

impl ResourceLoader for LuaScriptLoader {
    fn load_resource(
        &mut self,
        _handle: WrappedResourceLoader,
        _manager: ResourceManager,
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

        let script_src = match String::from_utf8(data) {
            Ok(text) => text,
            Err(_) => {
                return Err(ResourceError::new(
                    ResourceErrorReason::MalformedContent,
                    prototype.uid.as_str(),
                    "Lua script is not valid UTF-8",
                ))
            }
        };

        Ok(Box::into_raw(Box::new(LoadedScript::new(script_src))).cast())
    }

    fn copy_resource(
        &mut self,
        _handle: WrappedResourceLoader,
        _manager: ResourceManager,
        _prototype: ResourcePrototype,
        src_data: *const u8
    ) -> Result<*mut u8, ResourceError> {
        let script: &LoadedScript = unsafe { src_data.cast::<LoadedScript>().as_ref().unwrap() };
        Ok(Box::into_raw(Box::new(script.clone())).cast())
    }

    fn unload_resource(&mut self, _handle: WrappedResourceLoader, ptr: *mut u8) {
        unsafe {
            _ = Box::from_raw(ptr.cast::<LoadedScript>());
        }
    }
}
