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

use std::ptr;
use std::rc::Rc;
use resman_rs::{Resource, ResourceError, ResourceManager};
use render_rs::common::{Material, TextureData};
use crate::aglet::*;
use crate::state::RendererState;
use crate::util::gl_util::*;

pub(crate) fn get_or_load_texture(state: &mut RendererState, material_res: &Resource)
    -> Result<(), ResourceError> {
    let mat: &Material = material_res.get().unwrap();
    let texture_uid = mat.get_texture_uid();

    if let Some(tex) = state.prepared_textures.get(texture_uid) {
        state.material_textures.insert(
            material_res.get_prototype().uid.clone(),
            (texture_uid.to_string(), Rc::clone(tex)),
        );
        return Ok(());
    }

    let texture_res = ResourceManager::instance().get_resource(texture_uid)?;
    let texture: &TextureData = texture_res.get().unwrap();

    let width = texture.get_width() as i32;
    let height = texture.get_height() as i32;

    let mut handle: GlTextureHandle = 0;

    if aglet_have_gl_arb_direct_state_access() {
        glCreateTextures(GL_TEXTURE_2D, 1, &mut handle);

        glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR as i32);
        glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR as i32);
        glTextureParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE as i32);
        glTextureParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE as i32);
    } else {
        glGenTextures(1, &mut handle);
        bind_texture(0, handle);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR as i32);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR as i32);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE as i32);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE as i32);
    }

    //glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    if aglet_have_gl_arb_direct_state_access() {
        glTextureStorage2D(handle, 1, GL_RGBA8, width as GLsizei, height as GLsizei);
    } else if aglet_have_gl_arb_texture_storage() {
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width as GLsizei, height as GLsizei);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA as i32, width as GLsizei, height as GLsizei,
                0, GL_RGBA, GL_UNSIGNED_BYTE, ptr::null());
    }

    if aglet_have_gl_arb_direct_state_access() {
        glTextureSubImage2D(handle, 0, 0, 0, width as GLsizei, height as GLsizei,
            GL_RGBA, GL_UNSIGNED_BYTE, texture.get_pixel_data().as_ptr().cast());
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width as GLsizei, height as GLsizei,
            GL_RGBA, GL_UNSIGNED_BYTE, texture.get_pixel_data().as_ptr().cast());
    }

    if !aglet_have_gl_arb_direct_state_access() {
        bind_texture(0, 0);
    }

    let tex_rc = Rc::new(handle);
    state.prepared_textures.insert(texture_uid.to_string(), Rc::clone(&tex_rc));
    state.material_textures.insert(
        material_res.get_prototype().uid.clone(),
        (texture_uid.to_string(), Rc::clone(&tex_rc))
    );

    Ok(())
}

pub(crate) fn deinit_texture(texture: GlTextureHandle) {
    glDeleteTextures(1, &texture);
}

pub(crate) fn release_texture(state: &mut RendererState, texture_uid: &str) {
    // deinit the texture if this is the last reference to it
    if let Some(tex_rc) = state.prepared_textures.remove(texture_uid) {
        match Rc::try_unwrap(tex_rc) {
            Ok(tex) => {
                deinit_texture(tex)
                //TODO
                //debug("Released last handle on texture %s", texture_uid.c_str());
            },
            Err(rc) => {
                #[allow(unused)]
                let new_count = Rc::strong_count(&rc) - 1;
                //TODO
                //debug("Released handle on texture %s (new refcount = %lu)",
                //  texture_uid.c_str(), new_rc);
            }
        }
    }
}
