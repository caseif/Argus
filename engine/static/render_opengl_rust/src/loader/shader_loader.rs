/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
use std::any::Any;
use std::io::Read;
use resman_rs::*;
use render_rs::common::{Shader, ShaderStage};
use render_rs::constants::*;

pub(crate) struct ShaderLoader {
    //TODO
}

impl ShaderLoader {
    pub(crate) fn new() -> Self {
        Self {}
    }
}

impl ResourceLoader for ShaderLoader {
    fn load_resource(
        &self,
        _manager: &ResourceManager,
        prototype: &ResourcePrototype,
        reader: &mut dyn Read,
        _size: u64
    ) -> Result<Box<dyn Any + Send + Sync>, ResourceError> {
        let (shader_type, shader_stage) = match prototype.media_type.as_str() {
            RESOURCE_TYPE_SHADER_GLSL_VERT => (SHADER_TYPE_GLSL, ShaderStage::Vertex),
            RESOURCE_TYPE_SHADER_GLSL_FRAG => (SHADER_TYPE_GLSL, ShaderStage::Fragment),
            _ => panic!("Unrecognized shader media type {}", prototype.media_type),
        };

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

        let shader = Shader::new(prototype.uid.to_string(), shader_type, shader_stage, data);
        Ok(Box::new(shader))
    }
}
