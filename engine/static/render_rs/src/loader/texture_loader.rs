use std::any::Any;
use std::io::Read;
use resman_rs::*;
use rgb::ComponentBytes;
use crate::common::TextureData;

pub(crate) struct TextureLoader {}

impl ResourceLoader for TextureLoader {
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
        
        let image = match lodepng::decode32(data) {
            Ok(image) => image,
            Err(e) => {
                return Err(ResourceError::new(
                    ResourceErrorReason::MalformedContent,
                    prototype.uid.to_string(),
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
        Ok(Box::new(texture))
    }
}
