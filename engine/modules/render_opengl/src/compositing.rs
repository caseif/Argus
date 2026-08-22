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

use crate::aglet::*;
use crate::shaders::*;
use crate::state::*;
use crate::util::buffer::GlBuffer;
use crate::util::defines::*;
use crate::util::gl_util::*;
use std::cmp::{max, min};
use std::mem::swap;
use std::ops::DerefMut;
use argus_render::common::{AttachedViewport, Material, Viewport, ViewportCoordinateSpaceMode};
use argus_render::constants::*;
use argus_render::twod::{get_render_context_2d, AttachedViewport2d, Std140Light2D};
use argus_util::dirtiable::ValueAndDirtyFlag;
use argus_util::math::{Vector2u, Vector4f};
use crate::util::support::{GlExt, GlSupport};

const BINDING_INDEX_VBO: u32 = 0;

const LIGHT_ENVELOPE_BUFFER: f32 = 2.0;

struct TransformedViewport {
    pub(crate) top: i32,
    pub(crate) bottom: i32,
    pub(crate) left: i32,
    pub(crate) right: i32,
}

fn transform_viewport_to_pixels(viewport: &Viewport, resolution: &Vector2u) -> TransformedViewport {
    let min_dim = min(resolution.x, resolution.y) as f32;
    let max_dim = max(resolution.x, resolution.y) as f32;

    let (vp_h_scale, vp_v_scale, vp_h_off, vp_v_off): (f32, f32, f32, f32) = match viewport.mode {
        ViewportCoordinateSpaceMode::Individual => {
            (resolution.x as f32, resolution.y as f32, 0f32, 0f32)
        }
        ViewportCoordinateSpaceMode::MinAxis => (
            min_dim,
            min_dim,
            if resolution.x > resolution.y {
                (resolution.x - resolution.y) as f32 / 2f32
            } else {
                0f32
            },
            if resolution.y > resolution.x {
                (resolution.y - resolution.x) as f32 / 2f32
            } else {
                0f32
            },
        ),
        ViewportCoordinateSpaceMode::MaxAxis => (
            max_dim,
            max_dim,
            if resolution.x < resolution.y {
                (resolution.y - resolution.x) as f32 / -2f32
            } else {
                0f32
            },
            if resolution.y < resolution.x {
                (resolution.x - resolution.y) as f32 / -2f32
            } else {
                0f32
            },
        ),
        ViewportCoordinateSpaceMode::HorizontalAxis => (
            resolution.x as f32,
            resolution.x as f32,
            0f32,
            (resolution.y as f32 - resolution.x as f32) / 2f32,
        ),
        ViewportCoordinateSpaceMode::VerticalAxis => (
            resolution.y as f32,
            resolution.y as f32,
            (resolution.x as f32 - resolution.y as f32) / 2f32,
            0f32,
        ),
    };

    TransformedViewport {
        left: (viewport.left * vp_h_scale + vp_h_off) as i32,
        right: (viewport.right * vp_h_scale + vp_h_off) as i32,
        top: (viewport.top * vp_v_scale + vp_v_off) as i32,
        bottom: (viewport.bottom * vp_v_scale + vp_v_off) as i32,
    }
}

fn update_scene_ubo_2d(scene_state: &mut Scene2dState) {
    let mut must_update = false;

    let ubo = scene_state.ubo.get_or_insert_with(|| {
        must_update = true;
        GlBuffer::new(
            GL_UNIFORM_BUFFER,
            SHADER_UBO_SCENE_LEN as usize,
            GL_DYNAMIC_DRAW,
            true,
        )
    });

    let (al_level, al_color) = {
        let mut scene = get_render_context_2d().get_scene_mut(&scene_state.scene_id).unwrap();
        (
            scene.get_ambient_light_level(),
            scene.get_ambient_light_color(),
        )
    };

    if must_update || al_level.dirty {
        ubo.write_val::<f32>(&al_level.value, SHADER_UNIFORM_SCENE_AL_LEVEL_OFF as usize);
    }

    if must_update || al_color.dirty {
        let color_rgba = Vector4f::new(al_color.value.x, al_color.value.y, al_color.value.z, 1f32);
        ubo.write_val(color_rgba, SHADER_UNIFORM_SCENE_AL_COLOR_OFF as usize);
    }
}

fn update_viewport_ubo(viewport: &mut AttachedViewport2d, scene_state: &Scene2dState, viewport_state: &mut ViewportState) {
    let mut must_update = viewport.is_view_state_dirty() || true; //TODO

    let ubo = viewport_state.buffers.ubo.get_or_insert_with(|| {
        must_update = true;
        GlBuffer::new(
            GL_UNIFORM_BUFFER,
            SHADER_UBO_VIEWPORT_LEN as usize,
            GL_DYNAMIC_DRAW,
            true,
        )
    });

    if must_update {
        ubo.write_vals(
            &viewport.get_view_matrix().value.cells,
            SHADER_UNIFORM_VIEWPORT_VM_OFF as usize,
        );
        ubo.write_vals(
            &viewport.get_view_matrix().value.inverse().unwrap().cells,
            SHADER_UNIFORM_VIEWPORT_VM_INV_OFF as usize,
        );

        let mut scene = get_render_context_2d().get_scene_mut(&scene_state.scene_id).unwrap();

        let light_handles = scene.get_lights_for_viewport(viewport, LIGHT_ENVELOPE_BUFFER);
        let lights_count = light_handles.len();

        let mut shader_lights_arr: [Std140Light2D; LIGHTS_MAX as usize] = Default::default();
        for (i, light_handle) in light_handles.into_iter().enumerate() {
            let light = scene.get_light(light_handle).unwrap();
            shader_lights_arr[i] = light.to_shader_repr();
        }

        ubo.write_val(
            lights_count,
            SHADER_UNIFORM_VIEWPORT_LIGHT_COUNT_OFF as usize,
        );

        ubo.write_vals(&shader_lights_arr, SHADER_UNIFORM_VIEWPORT_LIGHTS_OFF as usize);
    }
}

fn bind_ubo(program: &LinkedProgram, name: &str, buffer: &GlBuffer) {
    program
        .reflection
        .ubo_bindings
        .get(name)
        .inspect(|binding| {
            glBindBufferBase(GL_UNIFORM_BUFFER, **binding, buffer.get_handle());
        });
}

fn create_framebuffers(n: GLsizei) -> Vec<GlBufferHandle> {
    let mut handles = Vec::<GlBufferHandle>::with_capacity(n as usize);
    handles.resize(n as usize, Default::default());
    if GlSupport::have(GlExt::DirectStateAccess) {
        glCreateFramebuffers(n, handles.as_mut_ptr());
    } else {
        glGenFramebuffers(n, handles.as_mut_ptr());
    }
    handles
}

fn create_textures(target: GLenum, n: GLsizei) -> Vec<GlTextureHandle> {
    let mut handles = Vec::<GlTextureHandle>::with_capacity(n as usize);
    handles.resize(n as usize, Default::default());
    if GlSupport::have(GlExt::DirectStateAccess) {
        glCreateTextures(target, n, handles.as_mut_ptr());
    } else {
        glGenTextures(n, handles.as_mut_ptr());
        for i in 0..n {
            glBindTexture(target, handles[i as usize]);
        }
    }
    handles
}

pub(crate) fn draw_scene_2d_to_framebuffer(
    renderer_state: &mut RendererState,
    viewport_id: u32,
    resolution: &ValueAndDirtyFlag<Vector2u>,
) {
    let mut att_viewport = get_render_context_2d().get_viewport_mut(viewport_id)
        .expect("Viewport was missing from context!");

    let viewport_px = transform_viewport_to_pixels(att_viewport.get_viewport(), &resolution.value);

    let fb_width = (viewport_px.right - viewport_px.left).abs();
    let fb_height = (viewport_px.bottom - viewport_px.top).abs();

    let scene_id = att_viewport.get_scene_id().to_string();

    get_render_context_2d().get_scene_mut(&scene_id).unwrap().update_lights_quadtree();

    // set scene uniforms
    update_scene_ubo_2d(
        renderer_state.scene_states_2d.get_mut(&scene_id).unwrap()
    );

    // set viewport uniforms
    update_viewport_ubo(
        att_viewport.deref_mut(),
        renderer_state.scene_states_2d.get(&scene_id).expect("Scene state was missing!"),
        renderer_state.viewport_states_2d.get_mut(&viewport_id).unwrap(),
    );

    let scene_state = renderer_state.scene_states_2d.get(&scene_id).unwrap();

    init_viewport_buffers(
        renderer_state.viewport_states_2d.get_mut(&viewport_id).unwrap(),
        resolution,
        fb_width,
        fb_height,
    );

    let fb_prim = renderer_state.viewport_states_2d
        .get(&att_viewport.get_id()).unwrap().buffers.fb_primary.unwrap();
    let fb_sec = renderer_state.viewport_states_2d
        .get(&att_viewport.get_id()).unwrap().buffers.fb_secondary.unwrap();

    let viewport_state = renderer_state.viewport_states_2d
        .get_mut(&att_viewport.get_id()).unwrap();

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_prim);

    // clear framebuffer
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(
        -viewport_px.left,
        -viewport_px.top,
        resolution.value.x as GLsizei,
        resolution.value.y as GLsizei,
    );

    let mut last_program: GlProgramHandle = 0;
    let mut last_texture: GlTextureHandle = 0;

    let mut non_std_buckets = Vec::<RenderBucketKey>::new();

    for (key, bucket) in &scene_state.render_buckets {
        let mat: &Material = bucket.material_res.get().unwrap();
        let program_info = renderer_state
            .linked_programs
            .get(&bucket.material_res.get_prototype().uid)
            .unwrap();
        let texture_uid = mat.get_texture_uid();
        let tex_handle = renderer_state.prepared_textures.get(texture_uid).unwrap();

        if !GlSupport::have(GlExt::DrawBuffersBlend) || program_info.has_custom_frag {
            non_std_buckets.push(key.clone());
        }

        if program_info.handle != last_program {
            glUseProgram(program_info.handle);
            last_program = program_info.handle;

            bind_ubo(
                program_info,
                SHADER_UBO_GLOBAL,
                renderer_state.global_ubo.as_ref().unwrap(),
            );
            bind_ubo(
                program_info,
                SHADER_UBO_SCENE,
                scene_state.ubo.as_ref().unwrap(),
            );
            bind_ubo(
                program_info,
                SHADER_UBO_VIEWPORT,
                viewport_state.buffers.ubo.as_ref().unwrap(),
            );
        }

        if program_info
            .reflection
            .ubo_bindings
            .contains_key(SHADER_UBO_OBJ)
        {
            bind_ubo(
                program_info,
                SHADER_UBO_OBJ,
                bucket.obj_ubo.as_ref().unwrap(),
            );
        }

        if tex_handle.as_ref() != &last_texture {
            bind_texture(GL_TEXTURE_2D, 0, *tex_handle.as_ref());
            last_texture = *tex_handle.as_ref();
        }

        glBindVertexArray(bucket.vertex_array.unwrap());

        //TODO: move this to material init
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST as GLint);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST as GLint);

        glDrawArrays(GL_TRIANGLES, 0, bucket.vertex_count as GLsizei);

        glBindVertexArray(0);
    }

    if !GlSupport::have(GlExt::DirectStateAccess) {
        bind_texture(GL_TEXTURE_2D, 0, 0);
    }

    let scene = get_render_context_2d().get_scene(&scene_state.scene_id).unwrap();
    if scene.is_lighting_enabled() {
        let light_handles = scene.get_lights_for_viewport(&att_viewport, LIGHT_ENVELOPE_BUFFER);
        let lights_count = light_handles.len();
        // generate shadowmap
        let shadowmap_program = get_shadowmap_program(&mut renderer_state.shadowmap_program);
        compute_scene_2d_shadowmap(
            scene_state,
            viewport_state,
            shadowmap_program,
            renderer_state.frame_vao.unwrap(),
            lights_count,
            resolution,
        );

        // generate lightmap
        let lighting_program = get_lighting_program(&mut renderer_state.lighting_program);
        draw_scene_2d_lightmap(
            scene_state,
            viewport_state,
            lighting_program,
            renderer_state.frame_vao.unwrap(),
            resolution,
        );

        // lightmaps are composited in a later step after this function is called

        draw_lightmap_to_framebuffer(
            renderer_state,
            att_viewport.get_id(),
            att_viewport.get_id(),
            att_viewport.get_viewport(),
            &resolution.value,
        );
    }

    let viewport_state = renderer_state.viewport_states_2d
        .get_mut(&att_viewport.get_id()).unwrap();

    // set buffers for ping-ponging
    let mut fb_front = fb_prim;
    let mut fb_back = fb_sec;
    let mut color_buf_front = viewport_state.buffers.color_buf_primary.unwrap();
    let mut color_buf_back = viewport_state.buffers.color_buf_secondary.unwrap();

    for postfx in att_viewport.get_postprocessing_shaders() {
        let postfx_programs = &mut renderer_state.postfx_programs;

        let postfx_program = postfx_programs
            .entry(postfx.clone())
            .or_insert_with_key(|postfx| link_program([SHADER_FB_VERT, postfx.as_str()]));

        swap(&mut fb_front, &mut fb_back);
        swap(&mut color_buf_front, &mut color_buf_back);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_front);

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, fb_width, fb_height);

        glBindVertexArray(renderer_state.frame_vao.unwrap());
        glUseProgram(postfx_program.handle);
        bind_texture(GL_TEXTURE_2D, 0, color_buf_back);

        bind_ubo(
            postfx_program,
            SHADER_UBO_GLOBAL,
            renderer_state.global_ubo.as_ref().unwrap(),
        );
        bind_ubo(
            postfx_program,
            SHADER_UBO_SCENE,
            scene_state.ubo.as_ref().unwrap(),
        );
        bind_ubo(
            postfx_program,
            SHADER_UBO_VIEWPORT,
            viewport_state.buffers.ubo.as_ref().unwrap(),
        );

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);

    viewport_state.buffers.color_buf_front = Some(color_buf_front);
    viewport_state.buffers.fb_primary = Some(fb_front);
    viewport_state.buffers.fb_secondary = Some(fb_back);

    // do selective second pass to populate auxiliary buffers
    if !non_std_buckets.is_empty() {
        // trick borrow checker to not tie mutable borrow to returned reference
        let std_program = get_std_program(&mut renderer_state.std_program);

        let mut last_tex: String = Default::default();

        if !GlSupport::have(GlExt::DrawBuffersBlend) {
            glBlendEquation(GL_MAX);
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.buffers.fb_aux.unwrap());

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(
            -viewport_px.left,
            -viewport_px.top,
            resolution.value.x as GLsizei,
            resolution.value.y as GLsizei,
        );

        glUseProgram(std_program.handle);

        bind_ubo(
            std_program,
            SHADER_UBO_GLOBAL,
            renderer_state.global_ubo.as_ref().unwrap(),
        );
        bind_ubo(
            std_program,
            SHADER_UBO_SCENE,
            scene_state.ubo.as_ref().unwrap(),
        );
        bind_ubo(
            std_program,
            SHADER_UBO_VIEWPORT,
            viewport_state.buffers.ubo.as_ref().unwrap(),
        );

        for key in &non_std_buckets {
            let bucket = &scene_state.render_buckets[key];

            bind_ubo(
                std_program,
                SHADER_UBO_OBJ,
                bucket.obj_ubo.as_ref().unwrap(),
            );

            let mat: &Material = bucket.material_res.get().unwrap();
            let texture_uid = mat.get_texture_uid();

            if texture_uid != &last_tex {
                let tex_handle = renderer_state.prepared_textures.get(texture_uid).unwrap();
                bind_texture(GL_TEXTURE_2D, 0, *tex_handle.as_ref());
                last_tex = texture_uid.to_string();
            }

            glBindVertexArray(bucket.vertex_array.unwrap());

            //TODO: move this to material init
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST as GLint);

            glDrawArrays(GL_TRIANGLES, 0, bucket.vertex_count as GLsizei);
        }

        glBindVertexArray(0);

        if !GlSupport::have(GlExt::DirectStateAccess) {
            bind_texture(GL_TEXTURE_2D, 0, 0);
        }

        glUseProgram(0);

        if !GlSupport::have(GlExt::DrawBuffersBlend) {
            glBlendEquation(GL_FUNC_ADD);
        }
    }

    bind_texture(GL_TEXTURE_2D, 0, 0);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

pub(crate) fn draw_lightmap_to_framebuffer(
    renderer_state: &RendererState,
    source_viewport_id: u32,
    target_viewport_id: u32,
    fb_viewport: &Viewport,
    resolution: &Vector2u,
) {
    let viewport_px = transform_viewport_to_pixels(fb_viewport, resolution);
    let fb_width = (viewport_px.right - viewport_px.left).abs();
    let fb_height = (viewport_px.bottom - viewport_px.top).abs();

    let fb_prim = renderer_state.viewport_states_2d
        .get(&target_viewport_id).unwrap().buffers.fb_primary.unwrap();
    let lightmap_buf = renderer_state.viewport_states_2d
        .get(&source_viewport_id).unwrap().buffers.lightmap_buf.unwrap();

    glUseProgram(renderer_state.frame_program.as_ref().unwrap().handle);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_prim);

    glViewport(0, 0, fb_width, fb_height);

    glBindVertexArray(renderer_state.frame_vao.unwrap());
    bind_texture(GL_TEXTURE_2D, 0, lightmap_buf);

    // blend color multiplicatively, don't touch destination alpha
    glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    restore_gl_blend_params();

    bind_texture(GL_TEXTURE_2D, 0, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glUseProgram(0);
}

fn init_viewport_buffers(
    viewport_state: &mut ViewportState,
    resolution: &ValueAndDirtyFlag<Vector2u>,
    fb_width: GLsizei,
    fb_height: GLsizei
) {
    // shadowmap setup
    if viewport_state.buffers.shadowmap_texture.is_none() {
        viewport_state.buffers.shadowmap_buffer = Some(GlBuffer::new(
            GL_TEXTURE_BUFFER,
            SHADER_IMAGE_SHADOWMAP_LEN,
            GL_STREAM_COPY,
            false,
        ));
        let sm_buf = viewport_state.buffers.shadowmap_buffer.as_ref().unwrap();
        if GlSupport::have(GlExt::DirectStateAccess) {
            let sm_tex = {
                let mut handle = 0;
                glCreateTextures(GL_TEXTURE_BUFFER, 1, &mut handle);
                viewport_state.buffers.shadowmap_texture = Some(handle);
                handle
            };
            glTextureBuffer(
                sm_tex,
                if GlSupport::have(GlExt::ComputeShader) { GL_R32F } else { GL_R32UI },
                sm_buf.get_handle(),
            );
            glTextureParameteri(sm_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST as GLint);
            glTextureParameteri(sm_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST as GLint);
        } else {
            let sm_tex = {
                let mut handle = 0;
                glGenTextures(1, &mut handle);
                viewport_state.buffers.shadowmap_texture = Some(handle);
                handle
            };
            bind_texture(GL_TEXTURE_BUFFER, 0, sm_tex);
            glTexBuffer(
                GL_TEXTURE_BUFFER,
                if GlSupport::have(GlExt::ComputeShader) { GL_R32F } else { GL_R32UI },
                sm_buf.get_handle(),
            );
            glTexParameteri(
                GL_TEXTURE_BUFFER,
                GL_TEXTURE_MIN_FILTER,
                GL_NEAREST as GLint,
            );
            glTexParameteri(
                GL_TEXTURE_BUFFER,
                GL_TEXTURE_MAG_FILTER,
                GL_NEAREST as GLint,
            );
        }
    }

    // framebuffer setup
    if viewport_state.buffers.fb_primary.is_none() {
        let framebufs = create_framebuffers(4);
        viewport_state.buffers.fb_primary = Some(framebufs[0]);
        viewport_state.buffers.fb_secondary = Some(framebufs[1]);
        viewport_state.buffers.fb_aux = Some(framebufs[2]);
        viewport_state.buffers.fb_lightmap = Some(framebufs[3]);
    }

    let fb_prim = viewport_state.buffers.fb_primary.unwrap();
    let fb_sec = viewport_state.buffers.fb_secondary.unwrap();
    let fb_aux = viewport_state.buffers.fb_aux.unwrap();
    let fb_lightmap = viewport_state.buffers.fb_lightmap.unwrap();

    if viewport_state.buffers.color_buf_primary.is_none() || resolution.dirty {
        viewport_state
            .buffers
            .color_buf_primary
            .take()
            .inspect(|handle| glDeleteTextures(1, handle));
        viewport_state
            .buffers
            .color_buf_secondary
            .take()
            .inspect(|handle| glDeleteTextures(1, handle));
        viewport_state
            .buffers
            .light_opac_map_buf
            .take()
            .inspect(|handle| glDeleteTextures(1, handle));
        viewport_state
            .buffers
            .lightmap_buf
            .take()
            .inspect(|handle| glDeleteTextures(1, handle));

        let tex_handles = create_textures(GL_TEXTURE_2D, 4);
        let cb_prim = tex_handles[0];
        let cb_sec = tex_handles[1];
        let lom_buf = tex_handles[2];
        let lm_buf = tex_handles[3];

        viewport_state.buffers.color_buf_primary = Some(cb_prim);
        viewport_state.buffers.color_buf_secondary = Some(cb_sec);
        viewport_state.buffers.light_opac_map_buf = Some(lom_buf);
        viewport_state.buffers.lightmap_buf = Some(lm_buf);

        if GlSupport::have(GlExt::DirectStateAccess) {
            // initialize color buffers

            alloc_texture_2d(cb_prim, GL_RGBA8, fb_width, fb_height);
            glTextureParameteri(cb_prim, GL_TEXTURE_MIN_FILTER, GL_LINEAR as GLint);
            glTextureParameteri(cb_prim, GL_TEXTURE_MAG_FILTER, GL_LINEAR as GLint);

            alloc_texture_2d(cb_sec, GL_RGBA8, fb_width, fb_height);
            glTextureParameteri(cb_sec, GL_TEXTURE_MIN_FILTER, GL_LINEAR as GLint);
            glTextureParameteri(cb_sec, GL_TEXTURE_MAG_FILTER, GL_LINEAR as GLint);

            // initialize auxiliary buffers
            alloc_texture_2d(lom_buf, GL_R32F, fb_width, fb_height);
            glTextureParameteri(lom_buf, GL_TEXTURE_MIN_FILTER, GL_NEAREST as GLint);
            glTextureParameteri(lom_buf, GL_TEXTURE_MAG_FILTER, GL_NEAREST as GLint);

            alloc_texture_2d(lm_buf, GL_RGBA8, fb_width, fb_height);
            glTextureParameteri(lm_buf, GL_TEXTURE_MIN_FILTER, GL_NEAREST as GLint);
            glTextureParameteri(lm_buf, GL_TEXTURE_MAG_FILTER, GL_NEAREST as GLint);

            // attach primary color buffers
            glNamedFramebufferTexture(fb_prim, GL_COLOR_ATTACHMENT0, cb_prim, 0);
            // need to be able to set a per-attachment blend
            // function + equation to be able to do it in one pass
            if GlSupport::have(GlExt::DrawBuffersBlend) {
                let draw_bufs = [GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1];
                glNamedFramebufferDrawBuffers(fb_prim, 2, draw_bufs.as_ptr());
            }

            // attach auxiliary buffers
            glNamedFramebufferTexture(fb_prim, GL_COLOR_ATTACHMENT1, lom_buf, 0);
            // don't attach aux buffers to the secondary fb so they don't get
            // lost while ping-ponging

            // attach secondary color buffer
            glNamedFramebufferTexture(fb_sec, GL_COLOR_ATTACHMENT0, cb_sec, 0);

            // set up second-pass auxiliary FBO
            glNamedFramebufferTexture(fb_aux, GL_COLOR_ATTACHMENT1, lom_buf, 0);

            let aux_draw_bufs = [GL_NONE, GL_COLOR_ATTACHMENT1];
            glNamedFramebufferDrawBuffers(fb_aux, 2, aux_draw_bufs.as_ptr());
            glNamedFramebufferReadBuffer(fb_aux, GL_NONE);

            // set up framebuffer for lighting pass
            glNamedFramebufferTexture(fb_lightmap, GL_COLOR_ATTACHMENT0, lm_buf, 0);

            let lightmap_draw_bufs = [GL_COLOR_ATTACHMENT0];
            glNamedFramebufferDrawBuffers(fb_lightmap, 1, lightmap_draw_bufs.as_ptr());

            // check framebuffer statuses

            let fb_front_status = glCheckNamedFramebufferStatus(fb_prim, GL_FRAMEBUFFER);
            if fb_front_status != GL_FRAMEBUFFER_COMPLETE {
                panic!(
                    "Front framebuffer is incomplete (error {})",
                    fb_front_status
                );
            }

            let fb_back_status = glCheckNamedFramebufferStatus(fb_sec, GL_FRAMEBUFFER);
            if fb_back_status != GL_FRAMEBUFFER_COMPLETE {
                panic!("Back framebuffer is incomplete (error {})", fb_back_status);
            }

            let fb_aux_status = glCheckNamedFramebufferStatus(fb_aux, GL_FRAMEBUFFER);
            if fb_aux_status != GL_FRAMEBUFFER_COMPLETE {
                panic!(
                    "Opacity map framebuffer is incomplete (error {})",
                    fb_aux_status
                );
            }

            let fb_lm_status = glCheckNamedFramebufferStatus(fb_lightmap, GL_FRAMEBUFFER);
            if fb_lm_status != GL_FRAMEBUFFER_COMPLETE {
                panic!(
                    "Lightmap framebuffer is incomplete (error {})",
                    fb_lm_status
                );
            }
        } else {
            // initialize color buffers

            bind_texture(GL_TEXTURE_2D, 0, cb_prim);
            alloc_texture_2d(GL_TEXTURE_2D, GL_RGBA8, fb_width, fb_height);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR as GLint);

            bind_texture(GL_TEXTURE_2D, 0, cb_sec);
            alloc_texture_2d(GL_TEXTURE_2D, GL_RGBA8, fb_width, fb_height);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR as GLint);

            // initialize auxiliary buffers

            bind_texture(GL_TEXTURE_2D, 0, lom_buf);
            alloc_texture_2d(GL_TEXTURE_2D, GL_R32F, fb_width, fb_height);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST as GLint);

            bind_texture(GL_TEXTURE_2D, 0, lm_buf);
            alloc_texture_2d(GL_TEXTURE_2D, GL_RGBA8, fb_width, fb_height);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST as GLint);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST as GLint);

            glBindFramebuffer(GL_FRAMEBUFFER, fb_prim);
            // attach primary color buffer
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cb_prim, 0);
            // attach auxiliary lightmap buffer
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, lom_buf, 0);

            // need to be able to set a per-attachment blend
            // function + equation to be able to do it in one pass
            if GlSupport::have(GlExt::DrawBuffersBlend) {
                let draw_bufs = [GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1];
                glDrawBuffers(draw_bufs.len() as GLsizei, draw_bufs.as_ptr());
            }

            // attach secondary color buffer
            glBindFramebuffer(GL_FRAMEBUFFER, fb_sec);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cb_sec, 0);

            // don't attach aux buffers to the secondary fb so they don't get
            // lost while ping-ponging

            // set up second-pass auxiliary FBO
            glBindFramebuffer(GL_FRAMEBUFFER, fb_aux);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, lom_buf, 0);

            let aux_draw_bufs = [GL_NONE, GL_COLOR_ATTACHMENT1];
            glDrawBuffers(aux_draw_bufs.len() as GLsizei, aux_draw_bufs.as_ptr());
            glReadBuffer(GL_NONE);

            // set up framebuffer for lighting pass
            glBindFramebuffer(GL_FRAMEBUFFER, fb_lightmap);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lm_buf, 0);

            let lightmap_draw_bufs = [GL_COLOR_ATTACHMENT0];
            glDrawBuffers(lightmap_draw_bufs.len() as GLsizei, lightmap_draw_bufs.as_ptr());

            // check framebuffer statuses

            glBindFramebuffer(GL_FRAMEBUFFER, fb_prim);
            let fb_prim_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if fb_prim_status != GL_FRAMEBUFFER_COMPLETE {
                panic!(
                    "Front framebuffer is incomplete (error {})",
                    fb_prim_status
                );
            }

            glBindFramebuffer(GL_FRAMEBUFFER, fb_sec);
            let fb_sec_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if fb_sec_status != GL_FRAMEBUFFER_COMPLETE {
                panic!("Back framebuffer is incomplete (error {})", fb_sec_status);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, fb_aux);
            let fb_aux_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if fb_aux_status != GL_FRAMEBUFFER_COMPLETE {
                panic!(
                    "Opacity map framebuffer is incomplete (error {})",
                    fb_aux_status
                );
            }

            glBindFramebuffer(GL_FRAMEBUFFER, fb_lightmap);
            let fb_lightmap_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if fb_lightmap_status != GL_FRAMEBUFFER_COMPLETE {
                panic!(
                    "Lightmap framebuffer is incomplete (error {})",
                    fb_lightmap_status
                );
            }
        }
    }
}

fn compute_scene_2d_shadowmap(
    scene_state: &Scene2dState,
    viewport_state: &ViewportState,
    program: &LinkedProgram,
    frame_vao: GlArrayHandle,
    lights_count: usize,
    #[allow(unused)]
    resolution: &ValueAndDirtyFlag<Vector2u>,
) {
    viewport_state
        .buffers
        .shadowmap_buffer
        .as_ref()
        .unwrap()
        .clear(u32::MAX);

    // the shadowmap shader discards fragments unconditionally
    // so we can just reuse the secondary framebuffer
    glBindFramebuffer(
        GL_DRAW_FRAMEBUFFER,
        viewport_state.buffers.fb_secondary.unwrap(),
    );
    glBindVertexArray(frame_vao);
    glUseProgram(program.handle);

    bind_ubo(
        program,
        SHADER_UBO_SCENE,
        scene_state.ubo.as_ref().unwrap(),
    );
    bind_ubo(
        program,
        SHADER_UBO_VIEWPORT,
        viewport_state.buffers.ubo.as_ref().unwrap(),
    );

    bind_texture(GL_TEXTURE_2D, 0, viewport_state.buffers.light_opac_map_buf.unwrap());

    glBindImageTexture(
        0,
        viewport_state.buffers.shadowmap_texture.unwrap(),
        0,
        GL_TRUE as GLboolean,
        0,
        GL_READ_WRITE,
        if GlSupport::have(GlExt::ComputeShader) { GL_R32F } else { GL_R32UI },
    );

    if GlSupport::have(GlExt::ComputeShader) {
        let workgroup_count = (lights_count * SHADOW_RAYS_COUNT + SHADOW_WORKGROUPS - 1) as f32 /
            SHADOW_WORKGROUPS as f32;
        glDispatchCompute(workgroup_count as u32, 1, 1);
    } else {
        // compute shaders not supported, use fragment shader fallback
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    glUseProgram(0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    //std::swap(viewport_state.fb_primary, viewport_state.fb_secondary);
    //std::swap(viewport_state.color_buf_primary, viewport_state.color_buf_secondary);
}

fn draw_scene_2d_lightmap(
    scene_state: &Scene2dState,
    viewport_state: &ViewportState,
    lighting_program: &LinkedProgram,
    frame_vao: GlArrayHandle,
    #[allow(unused)]
    resolution: &ValueAndDirtyFlag<Vector2u>,
) {
    glBindFramebuffer(
        GL_DRAW_FRAMEBUFFER,
        viewport_state.buffers.fb_lightmap.unwrap(),
    );
    glBindVertexArray(frame_vao);
    glUseProgram(lighting_program.handle);

    glClearColor(1f32, 1f32, 1f32, 0f32);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bind_ubo(
        lighting_program,
        SHADER_UBO_SCENE,
        scene_state.ubo.as_ref().unwrap(),
    );
    bind_ubo(
        lighting_program,
        SHADER_UBO_VIEWPORT,
        viewport_state.buffers.ubo.as_ref().unwrap(),
    );

    bind_texture(GL_TEXTURE_BUFFER, 0, viewport_state.buffers.shadowmap_texture.unwrap());

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);

    glUseProgram(0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

pub(crate) fn draw_framebuffer_to_screen(
    viewport_state: &ViewportState,
    viewport: &Viewport,
    frame_program: &LinkedProgram,
    frame_vao: GlArrayHandle,
    resolution: &ValueAndDirtyFlag<Vector2u>,
) {
    let viewport_px =
        transform_viewport_to_pixels(viewport, &resolution.value);
    let viewport_width_px = (viewport_px.right - viewport_px.left).abs();
    let viewport_height_px = (viewport_px.bottom - viewport_px.top).abs();

    let viewport_y = resolution.value.y as GLsizei - viewport_px.bottom;

    glViewport(
        viewport_px.left,
        viewport_y as GLsizei,
        viewport_width_px,
        viewport_height_px,
    );

    glBindVertexArray(frame_vao);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glUseProgram(frame_program.handle);
    bind_texture(GL_TEXTURE_2D, 0, viewport_state.buffers.color_buf_front.unwrap());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    bind_texture(GL_TEXTURE_2D, 0, 0);
    glUseProgram(0);
    glBindVertexArray(0);
}

pub(crate) fn setup_framebuffer(state: &mut RendererState) {
    let frame_program = link_program([SHADER_FB_VERT, SHADER_FB_FRAG]);

    if !frame_program
        .reflection
        .inputs
        .contains_key(SHADER_ATTRIB_POSITION)
    {
        panic!("Frame program is missing required position attribute");
    }
    if !frame_program
        .reflection
        .inputs
        .contains_key(SHADER_ATTRIB_TEXCOORD)
    {
        panic!("Frame program is missing required texcoords attribute");
    }

    state.frame_program = Some(frame_program);

    let frame_quad_vertex_data: [f32; 24] = [
        -1.0, -1.0, 0.0, 0.0,
        -1.0, 1.0, 0.0, 1.0,
        1.0, 1.0, 1.0, 1.0,
        -1.0, -1.0, 0.0, 0.0,
        1.0, 1.0, 1.0, 1.0,
        1.0, -1.0, 1.0, 0.0,
    ];

    let stride = 4 * size_of::<GLfloat>() as GLsizei;
    if GlSupport::have(GlExt::DirectStateAccess) {
        let frame_vao = {
            let mut handle = 0;
            glCreateVertexArrays(1, &mut handle);
            state.frame_vao = Some(handle);
            handle
        };

        let frame_vbo = {
            let mut handle = 0;
            glCreateBuffers(1, &mut handle);
            state.frame_vbo = Some(handle);
            handle
        };

        glNamedBufferData(
            frame_vbo,
            size_of_val(&frame_quad_vertex_data) as GLsizeiptr,
            frame_quad_vertex_data.as_ptr().cast(),
            GL_STATIC_DRAW,
        );

        glVertexArrayVertexBuffer(
            frame_vao,
            BINDING_INDEX_VBO,
            frame_vbo,
            0,
            stride,
        );
    } else {
        let frame_vao = {
            let mut handle = 0;
            glGenVertexArrays(1, &mut handle);
            state.frame_vao = Some(handle);
            handle
        };
        glBindVertexArray(frame_vao);

        let frame_vbo = {
            let mut handle = 0;
            glGenBuffers(1, &mut handle);
            state.frame_vbo = Some(handle);
            handle
        };
        glBindBuffer(GL_ARRAY_BUFFER, frame_vbo);

        glBufferData(
            GL_ARRAY_BUFFER,
            size_of_val(&frame_quad_vertex_data) as GLsizeiptr,
            frame_quad_vertex_data.as_ptr().cast(),
            GL_STATIC_DRAW,
        );

        if GlSupport::have(GlExt::VertexAttribBinding) {
            glBindVertexBuffer(BINDING_INDEX_VBO, frame_vbo, 0, stride);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, frame_vbo);
        }
    }

    let mut attr_offset = 0;
    set_attrib_pointer(
        state.frame_vao.unwrap(),
        state.frame_vbo.unwrap(),
        BINDING_INDEX_VBO,
        4,
        SHADER_ATTRIB_POSITION_LEN as GLuint,
        FB_SHADER_ATTRIB_POSITION_LOC,
        &mut attr_offset,
    );
    set_attrib_pointer(
        state.frame_vao.unwrap(),
        state.frame_vbo.unwrap(),
        BINDING_INDEX_VBO,
        4,
        SHADER_ATTRIB_TEXCOORD_LEN as GLuint,
        FB_SHADER_ATTRIB_TEXCOORD_LOC,
        &mut attr_offset,
    );

    if !GlSupport::have(GlExt::DirectStateAccess) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}
