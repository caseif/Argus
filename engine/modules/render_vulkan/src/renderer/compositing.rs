use crate::state::{PerFrameData, RendererState, ViewportState};
use argus_render::common::{AttachedViewport, SceneType, Viewport, ViewportCoordinateSpaceMode};
use argus_render::constants::*;
use argus_render::twod::get_render_context_2d;
use argus_util::dirtiable::ValueAndDirtyFlag;
use argus_util::math::{Matrix4x4, Vector2u};
use vk_wrapper::*;

struct TransformedViewport {
    top: i32,
    #[allow(unused)]
    bottom: i32,
    left: i32,
    #[allow(unused)]
    right: i32,
}

fn transform_viewport_to_pixels(viewport: &Viewport, resolution: &Vector2u)
    -> TransformedViewport {
    let res_x_f32 = resolution.x as f32;
    let res_y_f32 = resolution.y as f32;
    let min_dim = resolution.x.min(resolution.y) as f32;
    let max_dim = resolution.x.max(resolution.y) as f32;

    let (vp_h_scale, vp_v_scale, vp_h_off, vp_v_off) = match viewport.mode {
        ViewportCoordinateSpaceMode::Individual => (
            res_x_f32,
            res_y_f32,
            0.0,
            0.0,
        ),
        ViewportCoordinateSpaceMode::MinAxis => (
            min_dim,
            min_dim,
            if res_x_f32 > res_y_f32 {
                (res_x_f32 - res_y_f32) / 2.0
            } else {
                0.0
            },
            if res_y_f32 > res_x_f32 {
                (res_y_f32 - res_x_f32) / 2.0
            } else {
                0.0
            },
        ),
        ViewportCoordinateSpaceMode::MaxAxis => (
            max_dim,
            max_dim,
            if res_x_f32 < res_y_f32 {
                (res_x_f32 - res_y_f32) / 2.0
            } else {
                0.0
            },
            if res_y_f32 < res_x_f32 {
                (res_y_f32 - res_x_f32) / 2.0
            } else {
                0.0
            },
        ),
        ViewportCoordinateSpaceMode::HorizontalAxis => (
            res_x_f32,
            res_x_f32,
            0.0,
            (res_y_f32 - res_x_f32) / 2.0,
        ),
        ViewportCoordinateSpaceMode::VerticalAxis => (
            res_y_f32,
            res_y_f32,
            (res_x_f32 - res_y_f32) / 2.0,
            0.0,
        ),
    };

    TransformedViewport {
        left: (viewport.left * vp_h_scale + vp_h_off) as i32,
        right: (viewport.right * vp_h_scale + vp_h_off) as i32,
        top: (viewport.top * vp_v_scale + vp_v_off) as i32,
        bottom: (viewport.bottom * vp_v_scale + vp_v_off) as i32,
    }
}

fn create_uniform_ds_write<'ctx>(
    ds: &'ctx vk::DescriptorSet<'ctx>,
    binding: u32,
    buffer: &'ctx vk::Buffer,
) -> vk::WriteDescriptorSet<'ctx> {
    vk::WriteDescriptorSet::default()
        .dst_set(ds)
        .dst_binding(binding)
        .dst_array_element(0)
        .descriptor_type(vk::DescriptorType::UNIFORM_BUFFER)
        .descriptor_count(1)
        .buffer_info([
            vk::DescriptorBufferInfo::new(buffer)
                .offset(0)
                .range(vk::WHOLE_SIZE)
        ])
}

fn update_scene_ubo(
    device: &vk::Device,
    scene_type: SceneType,
    scene_id: &str,
    frame_state: &mut PerFrameData,
) {
    if !frame_state.scene_ubo_dirty {
        return;
    }

    if scene_type == SceneType::TwoDim {
        let scene = get_render_context_2d().get_scene(scene_id).unwrap();

        let al_level = scene.peek_ambient_light_level();
        let al_color = scene.peek_ambient_light_color();

        let scene_ubo = frame_state.scene_ubo.as_mut().unwrap();

        let al_color_arr: [f32; 3] = al_color.into();
        scene_ubo.write(device, &al_color_arr, SHADER_UNIFORM_SCENE_AL_COLOR_OFF as vk::DeviceSize)
            .unwrap();

        scene_ubo.write(device, &[al_level], SHADER_UNIFORM_SCENE_AL_LEVEL_OFF as vk::DeviceSize)
            .unwrap();
    }

    frame_state.scene_ubo_dirty = false;
}

fn update_viewport_ubo<'ctx>(
    device: &'ctx vk::Device<'ctx>,
    frame_state: &mut PerFrameData<'ctx>,
    view_matrix: &Matrix4x4,
) {
    let mut must_update = frame_state.view_matrix_dirty;

    let viewport_ubo = frame_state.viewport_ubo
        .get_or_insert_with(|| {
            must_update = true;
            vk::Buffer::new(
                device,
                SHADER_UBO_VIEWPORT_LEN as vk::DeviceSize,
                vk::BufferUsageFlags::UNIFORM_BUFFER,
                vk::MEM_CLASS_DEVICE_RW,
            )
                .unwrap()
    });

    if must_update {
        viewport_ubo.write(
            device,
            &view_matrix.cells,
            SHADER_UNIFORM_VIEWPORT_VM_OFF as vk::DeviceSize,
        )
            .unwrap();
    }
}

fn create_framebuffers<'state, 'ctx>(
    swapchain: &vk::Swapchain<'ctx>,
    pipeline: &vk::Pipeline,
    render_pass: &vk::RenderPass<'ctx>,
    desc_pool: &'state vk::DescriptorPool<'ctx>,
    viewport_state: &'state mut ViewportState<'ctx>,
    size: &Vector2u,
) {
    let format = swapchain.image_format;

    for frame_state in &mut viewport_state.per_frame {
        let front_images = vec![
            vk::Image::create_with_view(
                swapchain.get_device(),
                format,
                Vector2u::new(size.x, size.y),
                {
                    vk::ImageUsageFlags::COLOR_ATTACHMENT |
                    vk::ImageUsageFlags::SAMPLED |
                    vk::ImageUsageFlags::TRANSFER_SRC |
                    vk::ImageUsageFlags::TRANSFER_DST
                },
                vk::ImageAspectFlags::COLOR,
            ).unwrap(),
            vk::Image::create_with_view(
                swapchain.get_device(),
                vk::Format::R32_SFLOAT,
                Vector2u::new(size.x, size.y),
                {
                    vk::ImageUsageFlags::COLOR_ATTACHMENT |
                    vk::ImageUsageFlags::SAMPLED |
                    vk::ImageUsageFlags::TRANSFER_SRC |
                    vk::ImageUsageFlags::TRANSFER_DST
                },
                vk::ImageAspectFlags::COLOR,
            ).unwrap(),
        ];

        let sampler_info = vk::SamplerCreateInfo::default()
            .mag_filter(vk::Filter::NEAREST)
            .min_filter(vk::Filter::NEAREST)
            .address_mode_u(vk::SamplerAddressMode::CLAMP_TO_EDGE)
            .address_mode_v(vk::SamplerAddressMode::CLAMP_TO_EDGE)
            .address_mode_w(vk::SamplerAddressMode::CLAMP_TO_EDGE)
            .anisotropy_enable(false)
            .max_anisotropy(0.0)
            .border_color(vk::BorderColor::INT_OPAQUE_BLACK)
            .unnormalized_coordinates(false)
            .compare_enable(false)
            .compare_op(vk::CompareOp::ALWAYS)
            .mipmap_mode(vk::SamplerMipmapMode::LINEAR)
            .mip_lod_bias(0.0)
            .min_lod(0.0)
            .max_lod(0.0);

        assert!(frame_state.composite_desc_sets.is_none());
        frame_state.composite_desc_sets = Some(vk::DescriptorSetGroup::create(
            swapchain.get_device(),
            desc_pool,
            &pipeline.reflection,
        ).unwrap());

        let front_sampler = vk::Sampler::create(swapchain.get_device(), &sampler_info)
            .expect("Failed to create framebuffer sampler");

        let front_fb = vk::Framebuffer::create_from_images(
            swapchain.get_device(),
            render_pass,
            front_images,
            Some(front_sampler),
        ).unwrap();

        let back_images = vec![
            vk::Image::create_with_view(
                swapchain.get_device(),
                format,
                Vector2u::new(size.x, size.y),
                {
                    vk::ImageUsageFlags::COLOR_ATTACHMENT |
                    vk::ImageUsageFlags::SAMPLED
                },
                vk::ImageAspectFlags::COLOR,
            ).unwrap(),
            vk::Image::create_with_view(
                swapchain.get_device(),
                vk::Format::R32_SFLOAT,
                Vector2u::new(size.x, size.y),
                {
                    vk::ImageUsageFlags::COLOR_ATTACHMENT |
                    vk::ImageUsageFlags::SAMPLED
                },
                vk::ImageAspectFlags::COLOR,
            ).unwrap(),
        ];
        let back_fb = vk::Framebuffer::create_from_images(
            swapchain.get_device(),
            render_pass,
            back_images,
            None,
        ).unwrap();

        let ds_writes = frame_state.composite_desc_sets.as_ref().unwrap().iter().map(|ds| {
            vk::WriteDescriptorSet::default()
                .dst_set(ds)
                .dst_binding(0)
                .dst_array_element(0)
                .descriptor_type(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
                .descriptor_count(1)
                .image_info([
                    vk::DescriptorImageInfo::default()
                        .image_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)
                        .image_view(front_fb.get_images().unwrap()[0].get_view().unwrap())
                        .sampler(front_fb.get_sampler().unwrap())
                ])
        }).collect();
        swapchain.get_device().update_descriptor_sets(ds_writes);

        assert!(frame_state.front_fb.is_none());
        assert!(frame_state.back_fb.is_none());
        frame_state.front_fb = Some(front_fb);
        frame_state.back_fb = Some(back_fb);
    }
}

pub(crate) fn draw_scene_to_framebuffer(
    state: &mut RendererState,
    viewport_id: u32,
    resolution: ValueAndDirtyFlag<Vector2u>,
    cur_frame: usize,
) {
    let device = state.device;

    let mut viewport = get_render_context_2d().get_viewport_mut(viewport_id).unwrap();

    let viewport_state = state.viewport_states_2d.get_mut(&viewport.get_id()).unwrap();
    let scene_state = state.scene_states_2d.get_mut(viewport.get_scene_id()).unwrap();

    let swapchain = &state.swapchain.as_ref().unwrap();

    let viewport_px = transform_viewport_to_pixels(viewport.get_viewport(), &resolution.value);

    //uint32_t fb_width = uint32_t(std::abs(viewport_px.right - viewport_px.left));
    //uint32_t fb_height = uint32_t(std::abs(viewport_px.bottom - viewport_px.top));
    let fb_width = swapchain.extent.width;
    let fb_height = swapchain.extent.height;

    // framebuffer setup
    if resolution.dirty {
        let _submit_lock = state.submit_mutex.lock().unwrap();
        let _gfx_queue_lock = device.queue_mutexes.graphics_family.lock().unwrap();
        let _present_queue_lock = if device.queues.present_family != device.queues.graphics_family {
            Some(device.queue_mutexes.present_family.lock().unwrap())
        } else {
            None
        };

        device.queues.graphics_family.wait_idle().unwrap();

        for frame_state in &mut viewport_state.per_frame {
            if let Some(front_fb) = frame_state.front_fb.take() {
                // delete framebuffers
                front_fb.destroy();
            }
            if let Some(back_fb) = frame_state.back_fb.take() {
                back_fb.destroy();
            }
            if let Some(composite_ds_group) = frame_state.composite_desc_sets.take() {
                composite_ds_group.destroy(state.desc_pool.as_ref().unwrap()).unwrap();
            }
        }
    }

    if viewport_state.per_frame[cur_frame].front_fb.is_none() {
        create_framebuffers(
            swapchain,
            state.composite_pipeline.as_ref().unwrap(),
            state.fb_render_pass.as_ref().unwrap(),
            state.desc_pool.as_ref().unwrap(),
            viewport_state,
            &Vector2u::new(fb_width, fb_height),
        );
    }

    let cur_frame_state = &mut viewport_state.per_frame[cur_frame];

    update_scene_ubo(
        device,
        scene_state.scene_type,
        &scene_state.scene_id,
        cur_frame_state,
    );
    update_viewport_ubo(
        device,
        cur_frame_state,
        &viewport.get_view_matrix().value,
    );

    cur_frame_state.command_buf.as_ref().unwrap().begin_oneshot_commands();

    let cmd_buf = cur_frame_state.command_buf.as_ref().unwrap();

    let color_clear_val = vk::ClearValue {
        color: vk::ClearColorValue { float32: [0.0, 0.0, 0.0, 0.0] },
    };

    let light_opac_clear_val = vk::ClearValue {
        color: vk::ClearColorValue { float32: [0.0, 0.0, 0.0, 0.0] },
    };

    let clear_vals = [color_clear_val, light_opac_clear_val];

    let rp_info = vk::RenderPassBeginInfo::default()
        .framebuffer(cur_frame_state.front_fb.as_ref().unwrap())
        .clear_values(&clear_vals)
        .render_pass(state.fb_render_pass.as_ref().unwrap())
        .render_area(vk::Rect2D {
            extent: vk::Extent2D { width: fb_width, height: fb_height },
            offset: vk::Offset2D { x: 0, y: 0 },
        });
    cmd_buf.cmd_begin_render_pass(&rp_info, vk::SubpassContents::INLINE);

    assert!(
        resolution.value.x <= i32::MAX as u32 && resolution.value.y <= i32::MAX as u32,
        "Resolution is too big for viewport",
    );

    let mut last_pipeline = None;
    //texture_handle_t last_texture = 0;

    /*VkImageSubresourceRange range{};
    range.layerCount = 1;
    range.baseArrayLayer = 0;
    range.levelCount = 1;
    range.baseMipLevel = 0;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;*/

    /*VkClearAttachment clear_att{};
    clear_att.clearValue = clear_val;
    //clear_att.colorAttachment =
    VkClearRect clear_rect{};
    clear_rect.rect.extent.width =
    vkCmdClearAttachments(vk_cmd_buf, 1, &clear_att, 1, &clear_rect);*/

    for bucket in scene_state.render_buckets.values() {
        let vertex_count = bucket.vertex_count;

        let mat = bucket.material_res.clone();
        let pipeline_info = state.material_pipelines.get(&mat.get_prototype().uid).unwrap();

        let texture_uid = state.material_textures.get(&mat.get_prototype().uid).unwrap();
        let texture = state.prepared_textures.get(texture_uid).unwrap();

        let shader_refl = &pipeline_info.reflection;

        //let texture_uid = mat.get<Material>().get_texture_uid();
        //let tex_handle = state.prepared_textures.find(texture_uid).second;

        let desc_sets = cur_frame_state.material_desc_sets
            .entry(mat.get_prototype().uid.clone())
            .or_insert_with(|| {
                let sets = vk::DescriptorSetGroup::create(
                    device,
                    state.desc_pool.as_ref().unwrap(),
                    shader_refl,
                )
                    .unwrap();

                let mut ds_writes = Vec::new();

                let ds = &sets[0];

                let sampler_info = vk::DescriptorImageInfo::default()
                    .image_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)
                    .image_view(texture.image.get_view().as_ref().unwrap())
                    .sampler(&texture.sampler);
                let sampler_infos = [sampler_info];

                let sampler_ds_write = vk::WriteDescriptorSet::default()
                    .dst_set(ds)
                    .dst_binding(0)
                    .dst_array_element(0)
                    .descriptor_type(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
                    .descriptor_count(1)
                    .image_info(sampler_infos);

                ds_writes.push(sampler_ds_write);

                let global_ubo = state.global_ubo.as_ref().unwrap();
                shader_refl.get_ubo_binding_and_then(
                    SHADER_UBO_GLOBAL,
                    |binding| ds_writes.push(create_uniform_ds_write(
                        ds,
                        binding,
                        global_ubo,
                    )),
                );
                let scene_ubo = cur_frame_state.scene_ubo.as_ref().unwrap();
                shader_refl.get_ubo_binding_and_then(
                    SHADER_UBO_SCENE,
                    |binding| ds_writes.push(create_uniform_ds_write(
                        ds,
                        binding,
                        scene_ubo,
                    )),
                );
                let vp_ubo = cur_frame_state.viewport_ubo.as_ref().unwrap();
                shader_refl.get_ubo_binding_and_then(
                    SHADER_UBO_VIEWPORT,
                    |binding| ds_writes.push(create_uniform_ds_write(
                        ds,
                        binding,
                        vp_ubo,
                    )),
                );
                let obj_ubo = bucket.ubo_buffer.as_ref().unwrap();
                shader_refl.get_ubo_binding_and_then(
                    SHADER_UBO_OBJ,
                    |binding| ds_writes.push(create_uniform_ds_write(
                        ds,
                        binding,
                        obj_ubo,
                    )),
                );

                device.update_descriptor_sets(ds_writes);

                sets
            },
        );

        let current_desc_set = &desc_sets[0];
        let current_desc_sets = [current_desc_set];

        if last_pipeline.is_none() ||
            last_pipeline.is_some_and(|last| last == pipeline_info) {
            cmd_buf.cmd_bind_pipeline(
                vk::PipelineBindPoint::GRAPHICS,
                pipeline_info,
            );
            last_pipeline = Some(pipeline_info);

            let vk_viewport = vk::Viewport::default()
                .width(fb_width as f32)
                .height(fb_height as f32)
                .x(-(viewport_px.left as f32))
                .y(-(viewport_px.top as f32));
            let vk_viewports = [vk_viewport];
            cmd_buf.cmd_set_viewport(0, &vk_viewports);

            let vk_scissor = vk::Rect2D::default()
                .extent(vk::Extent2D {
                    width: fb_width,
                    height: fb_height,
                })
                .offset(vk::Offset2D {
                    x: 0,
                    y: 0,
                });
            let vk_scissors = [vk_scissor];
            cmd_buf.cmd_set_scissor(0, &vk_scissors);
        }

        cmd_buf.cmd_bind_descriptor_sets(
            vk::PipelineBindPoint::GRAPHICS,
            &pipeline_info.layout,
            0,
            &current_desc_sets,
            &[],
        );

        //if (tex_handle != last_texture) {
        //    glBindTexture(GL_TEXTURE_2D, tex_handle);
        //    last_texture = tex_handle;
        //}

        let vertex_buffers = [
            bucket.vertex_buffer.as_ref().unwrap(),
            bucket.anim_frame_buffer.as_ref().unwrap(),
        ];
        let offsets = [0, 0];
        cmd_buf.cmd_bind_vertex_buffers(0, &vertex_buffers, &offsets);

        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        cmd_buf.cmd_draw(vertex_count, 1, 0, 0);
    }

    /*for (let postfx : viewport_state.viewport.get_postprocessing_shaders()) {
        LinkedProgram *postfx_program;

        let postfx_programs = state.postfx_programs;
        let it = postfx_programs.find(postfx);
        if (it != postfx_programs.end()) {
            postfx_program = &it.second;
        } else {
            let linked_program = link_program({FB_SHADER_VERT_PATH, postfx});
            let inserted = postfx_programs.insert({postfx, linked_program});
            postfx_program = &inserted.first.second;
        }

        std::swap(viewport_state.fb_primary, viewport_state.fb_secondary);
        std::swap(viewport_state.color_buf_primary, viewport_state.color_buf_secondary);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, viewport_state.fb_primary);

        //glClearColor(0.0, 0.0, 0.0, 0.0);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // set viewport

        glBindVertexArray(state.frame_vao);
        glUseProgram(postfx_program.handle);
        set_per_frame_global_uniforms(*postfx_program);
        glBindTexture(GL_TEXTURE_2D, viewport_state.color_buf_secondary);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }*/

    cmd_buf.cmd_end_render_pass();

    /*if (vkEndCommandBuffer(vk_cmd_buf) != VK_SUCCESS) {
        crash("Failed to record command buffer");
    }*/
    cur_frame_state.command_buf.as_ref().unwrap().end_commands();
    device.reset_fences(
        &[&cur_frame_state.composite_fence.unwrap()]
    )
        .unwrap();
    cur_frame_state.command_buf.as_ref().unwrap().queue_submit(
        state.submit_sender.as_ref().unwrap(),
        state.submit_mutex.as_ref(),
        state.swapchain.as_ref().unwrap(),
        &device.queues.graphics_family,
        vec![cur_frame_state.rebuild_semaphore.unwrap()],
        vec![vk::PipelineStageFlags::ALL_COMMANDS],
        vec![cur_frame_state.draw_semaphore.unwrap()],
        Some(cur_frame_state.composite_fence.unwrap()),
        None,
    )
        .unwrap();
}

pub(crate) fn draw_framebuffer_to_swapchain<'ctx>(
    frame_state: &mut PerFrameData<'ctx>,
    viewport: &Viewport,
    swapchain: &vk::Swapchain<'ctx>,
    pipeline: &vk::Pipeline<'ctx>,
    cmd_buf: &vk::CommandBuffer<'ctx>,
) {
    let resolution = swapchain.resolution;

    let viewport_px = transform_viewport_to_pixels(viewport, &resolution);

    let cur_desc_sets = [&frame_state.composite_desc_sets.as_ref().unwrap()[0]];

    let fb_width = swapchain.resolution.x;
    let fb_height = swapchain.resolution.y;

    let vk_viewport = vk::Viewport::default()
        .width(fb_width as f32)
        .height(fb_height as f32)
        .x(-(viewport_px.left as f32))
        .y(-(viewport_px.top as f32));
    cmd_buf.cmd_set_viewport(0, &[vk_viewport]);

    let scissor = vk::Rect2D::default()
        .extent(vk::Extent2D {
            width: fb_width,
            height: fb_height,
        })
        .offset(vk::Offset2D {
            x: 0,
            y: 0,
        });
    cmd_buf.cmd_set_scissor(0, &[scissor]);

    cmd_buf.cmd_bind_descriptor_sets(
        vk::PipelineBindPoint::GRAPHICS,
        &pipeline.layout,
        0,
        &cur_desc_sets,
        &[],
    );

    cmd_buf.cmd_draw(6, 1, 0, 0);
}
