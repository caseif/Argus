use std::ops::Deref;
use resman_rustabi::argus::resman::*;
use rgb::ComponentBytes;
use crate::common::TextureData;

pub(crate) struct TextureLoader {}

impl ResourceLoader for TextureLoader {
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
        
        let image = match lodepng::decode32(data) {
            Ok(image) => image,
            Err(e) => {
                return Err(ResourceError::new(
                    ResourceErrorReason::MalformedContent,
                    prototype.uid.as_str(),
                    e.to_string().as_str(),
                ))
            }
        };
        
        let texture = TextureData::new(
            image.width as u32,
            image.height as u32,
            4, // BPP
            image.buffer.as_bytes().into()
        );
        Ok(Box::into_raw(Box::new(texture)).cast())
    }

    fn copy_resource(
        &mut self,
        _handle: WrappedResourceLoader,
        _manager: ResourceManager,
        _prototype: ResourcePrototype,
        src_data: *const u8
    ) -> Result<*mut u8, ResourceError> {
        let texture: &TextureData = unsafe { *src_data.cast() };
        Ok(Box::into_raw(Box::new(texture.clone())).cast())
    }

    fn unload_resource(&mut self, handle: WrappedResourceLoader, ptr: *mut u8) {
        unsafe {
            _ = Box::from_raw(ptr.cast::<TextureData>());
        }
    }
}
