use std::mem::MaybeUninit;
use ash::vk;
use argus_render::common::{AttachedViewport, Matrix4x4, RenderCanvas, SceneType, Viewport, ViewportCoordinateSpaceMode};
use argus_render::constants::*;
use argus_render::twod::{get_render_context_2d, AttachedViewport2d};
use argus_util::dirtiable::ValueAndDirtyFlag;
use argus_util::math::Vector2u;
use crate::setup::device::VulkanDevice;
use crate::setup::instance::VulkanInstance;
use crate::setup::swapchain::VulkanSwapchain;
use crate::state::{PerFrameData, RendererState, ViewportState};
use crate::util::*;

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

fn create_uniform_ds_write<'a>(
    ds: vk::DescriptorSet,
    binding: u32,
    buffer: &VulkanBuffer,
    buf_info: &'a mut MaybeUninit<vk::DescriptorBufferInfo>,
) -> vk::WriteDescriptorSet<'a> {
    buf_info.write(
        vk::DescriptorBufferInfo::default()
            .buffer(unsafe { buffer.get_handle() })
            .offset(0)
            .range(vk::WHOLE_SIZE)
    );

    let mut ds_write = vk::WriteDescriptorSet::default()
        .dst_set(ds)
        .dst_binding(binding)
        .dst_array_element(0)
        .descriptor_type(vk::DescriptorType::UNIFORM_BUFFER)
        .descriptor_count(1);
    ds_write.descriptor_count = 1;
    ds_write.p_buffer_info = buf_info.as_ptr();

    ds_write
}

fn update_scene_ubo(
    device: &VulkanDevice,
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

fn update_viewport_ubo(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    frame_state: &mut PerFrameData,
    view_matrix: &Matrix4x4,
) {
    let mut must_update = frame_state.view_matrix_dirty;

    let viewport_ubo = frame_state.viewport_ubo
        .get_or_insert_with(|| {
            must_update = true;
            VulkanBuffer::new(
                instance,
                device,
                SHADER_UBO_VIEWPORT_LEN as vk::DeviceSize,
                vk::BufferUsageFlags::UNIFORM_BUFFER,
                MEM_CLASS_DEVICE_RW,
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

fn create_framebuffers(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    swapchain: &VulkanSwapchain,
    pipeline: &PipelineInfo,
    render_pass: vk::RenderPass,
    desc_pool: vk::DescriptorPool,
    viewport_state: &mut ViewportState,
    size: &Vector2u,
) {
    let format = swapchain.image_format;

    for frame_state in &mut viewport_state.per_frame {
        let front_images = vec![
            VulkanImage::create_image_with_view(
                instance,
                device,
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
            VulkanImage::create_image_with_view(
                instance,
                device,
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
        let front_handle = create_framebuffer_for_swapchain_images(
            device,
            render_pass,
            &front_images,
        ).unwrap();

        let back_images = vec![
            VulkanImage::create_image_with_view(
                instance,
                device,
                format,
                Vector2u::new(size.x, size.y),
                {
                    vk::ImageUsageFlags::COLOR_ATTACHMENT |
                    vk::ImageUsageFlags::SAMPLED
                },
                vk::ImageAspectFlags::COLOR,
            ).unwrap(),
            VulkanImage::create_image_with_view(
                instance,
                device,
                vk::Format::R32_SFLOAT,
                Vector2u::new(size.x, size.y),
                {
                    vk::ImageUsageFlags::COLOR_ATTACHMENT |
                    vk::ImageUsageFlags::SAMPLED
                },
                vk::ImageAspectFlags::COLOR,
            ).unwrap(),
        ];
        let back_handle = create_framebuffer_for_swapchain_images(
            device,
            render_pass,
            &back_images,
        ).unwrap();

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

        let front_sampler = unsafe {
            device.logical_device
                .create_sampler(&sampler_info, None)
                .expect("Failed to create framebuffer sampler")
        };

        assert!(frame_state.composite_desc_sets.is_empty());
        frame_state.composite_desc_sets = create_descriptor_sets(
            device,
            desc_pool,
            &pipeline.reflection,
        ).unwrap();

        let desc_image_info = vk::DescriptorImageInfo::default()
            .image_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)
            .image_view(unsafe { front_images[0].get_vk_image_view() })
            .sampler(front_sampler);
        let desc_image_infos = [desc_image_info];

        let mut ds_writes = Vec::with_capacity(frame_state.composite_desc_sets.len());
        for ds in &frame_state.composite_desc_sets {
            let sampler_ds_write = vk::WriteDescriptorSet::default()
                .dst_set(*ds)
                .dst_binding(0)
                .dst_array_element(0)
                .descriptor_type(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
                .descriptor_count(1)
                .image_info(&desc_image_infos);
            ds_writes.push(sampler_ds_write);
        }
        unsafe {
            device.logical_device.update_descriptor_sets(&ds_writes, &[]);
        }

        assert!(frame_state.front_fb.is_none());
        assert!(frame_state.back_fb.is_none());
        frame_state.front_fb = Some(FramebufferGrouping {
            handle: front_handle,
            images: front_images,
            sampler: Some(front_sampler),
        });
        frame_state.back_fb = Some(FramebufferGrouping {
            handle: back_handle,
            images: back_images,
            sampler: None,
        });
    }
}

pub(crate) fn draw_scene_to_framebuffer(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    state: &mut RendererState,
    viewport_id: u32,
    resolution: ValueAndDirtyFlag<Vector2u>,
    cur_frame: usize,
) {
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

        unsafe { device.logical_device.queue_wait_idle(device.queues.graphics_family).unwrap() };
        for frame_state in &mut viewport_state.per_frame {
            if let Some(front_fb) = frame_state.front_fb.take() {
                // delete framebuffers
                destroy_framebuffer(device, front_fb.handle);
                for image in front_fb.images {
                    image.destroy(device);
                }
                if let Some(sampler) = front_fb.sampler {
                    unsafe { device.logical_device.destroy_sampler(sampler, None) };
                }
            }
            if let Some(back_fb) = frame_state.back_fb.take() {
                destroy_framebuffer(device, back_fb.handle);
                for image in back_fb.images {
                    image.destroy(device);
                }
                if let Some(sampler) = back_fb.sampler {
                    unsafe { device.logical_device.destroy_sampler(sampler, None) };
                }
            }
            if !frame_state.composite_desc_sets.is_empty() {
                destroy_descriptor_sets(
                    device,
                    state.desc_pool.unwrap(),
                    &frame_state.composite_desc_sets,
                )
                    .unwrap();
                frame_state.composite_desc_sets.clear();
            }
        }
    }

    if viewport_state.per_frame[cur_frame].front_fb.is_none() {
        create_framebuffers(
            instance,
            device,
            swapchain,
            state.composite_pipeline.as_ref().unwrap(),
            state.fb_render_pass.unwrap(),
            state.desc_pool.unwrap(),
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
        instance,
        device,
        cur_frame_state,
        &viewport.get_view_matrix().value,
    );

    begin_oneshot_commands(device, cur_frame_state.command_buf.as_ref().unwrap());

    let vk_cmd_buf = unsafe { cur_frame_state.command_buf.as_ref().unwrap().get_handle() };

    let color_clear_val = vk::ClearValue {
        color: vk::ClearColorValue { float32: [0.0, 0.0, 0.0, 0.0] },
    };

    let light_opac_clear_val = vk::ClearValue {
        color: vk::ClearColorValue { float32: [0.0, 0.0, 0.0, 0.0] },
    };

    let clear_vals = [color_clear_val, light_opac_clear_val];

    let rp_info = vk::RenderPassBeginInfo::default()
        .framebuffer(cur_frame_state.front_fb.as_ref().unwrap().handle)
        .clear_values(&clear_vals)
        .render_pass(state.fb_render_pass.unwrap())
        .render_area(vk::Rect2D {
            extent: vk::Extent2D { width: fb_width, height: fb_height },
            offset: vk::Offset2D { x: 0, y: 0 },
        });
    unsafe {
        device.logical_device
            .cmd_begin_render_pass(vk_cmd_buf, &rp_info, vk::SubpassContents::INLINE);
    }

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
                let sets = create_descriptor_sets(device, state.desc_pool.unwrap(), shader_refl)
                    .unwrap();

                let mut ds_writes = Vec::new();

                let ds = sets[0];

                let sampler_info = vk::DescriptorImageInfo::default()
                    .image_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL)
                    .image_view(unsafe { texture.image.get_vk_image_view() })
                    .sampler(texture.sampler);
                let sampler_infos = [sampler_info];

                let sampler_ds_write = vk::WriteDescriptorSet::default()
                    .dst_set(ds)
                    .dst_binding(0)
                    .dst_array_element(0)
                    .descriptor_type(vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
                    .image_info(&sampler_infos);

                ds_writes.push(sampler_ds_write);

                let global_ubo = state.global_ubo.as_ref().unwrap();
                let mut buf_info_global = MaybeUninit::uninit();
                let mut buf_info_scene = MaybeUninit::uninit();
                let mut buf_info_viewport = MaybeUninit::uninit();
                let mut buf_info_obj = MaybeUninit::uninit();
                shader_refl.get_ubo_binding_and_then(
                    SHADER_UBO_GLOBAL,
                    |binding| {
                        ds_writes.push(create_uniform_ds_write(ds, binding, global_ubo, &mut buf_info_global));
                    },
                );
                let scene_ubo = cur_frame_state.scene_ubo.as_ref().unwrap();
                shader_refl.get_ubo_binding_and_then(
                    SHADER_UBO_SCENE,
                    |binding| {
                        ds_writes.push(create_uniform_ds_write(ds, binding, scene_ubo, &mut buf_info_scene));
                    },
                );
                let vp_ubo = cur_frame_state.viewport_ubo.as_ref().unwrap();
                shader_refl.get_ubo_binding_and_then(
                    SHADER_UBO_VIEWPORT,
                    |binding| {
                        ds_writes.push(create_uniform_ds_write(ds, binding, vp_ubo, &mut buf_info_viewport));
                    },
                );
                let obj_ubo = bucket.ubo_buffer.as_ref().unwrap();
                shader_refl.get_ubo_binding_and_then(
                    SHADER_UBO_OBJ,
                    |binding| {
                        ds_writes.push(create_uniform_ds_write(ds, binding, obj_ubo, &mut buf_info_obj));
                    },
                );

                unsafe { device.logical_device.update_descriptor_sets(&ds_writes, &[]) };

                sets
            },
        );

        let current_desc_set = desc_sets[0];
        let current_desc_sets = [current_desc_set];

        if last_pipeline.is_none() ||
            last_pipeline.is_some_and(|last| last == unsafe { pipeline_info.get_handle() }) {
            unsafe {
                device.logical_device.cmd_bind_pipeline(
                    vk_cmd_buf,
                    vk::PipelineBindPoint::GRAPHICS,
                    pipeline_info.get_handle(),
                );
            }
            unsafe {
                last_pipeline = Some(pipeline_info.get_handle());
            }

            let vk_viewport = vk::Viewport::default()
                .width(fb_width as f32)
                .height(fb_height as f32)
                .x(-(viewport_px.left as f32))
                .y(-(viewport_px.top as f32));
            let vk_viewports = [vk_viewport];
            unsafe { device.logical_device.cmd_set_viewport(vk_cmd_buf, 0, &vk_viewports) };

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
            unsafe { device.logical_device.cmd_set_scissor(vk_cmd_buf, 0, &vk_scissors) };
        }

        unsafe {
            device.logical_device.cmd_bind_descriptor_sets(
                vk_cmd_buf,
                vk::PipelineBindPoint::GRAPHICS,
                pipeline_info.layout,
                0,
                &current_desc_sets,
                &[],
            );
        }

        //if (tex_handle != last_texture) {
        //    glBindTexture(GL_TEXTURE_2D, tex_handle);
        //    last_texture = tex_handle;
        //}

        unsafe {
            let vertex_buffers = [
                bucket.vertex_buffer.as_ref().unwrap().get_handle(),
                bucket.anim_frame_buffer.as_ref().unwrap().get_handle(),
            ];
            let offsets = [0, 0];
            device.logical_device.cmd_bind_vertex_buffers(vk_cmd_buf, 0, &vertex_buffers, &offsets);
        }

        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        unsafe { device.logical_device.cmd_draw(vk_cmd_buf, vertex_count, 1, 0, 0) };
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

    unsafe { device.logical_device.cmd_end_render_pass(vk_cmd_buf) };

    /*if (vkEndCommandBuffer(vk_cmd_buf) != VK_SUCCESS) {
        crash("Failed to record command buffer");
    }*/
    end_command_buffer(device, cur_frame_state.command_buf.as_ref().unwrap());
    unsafe {
        device.logical_device.reset_fences(&[cur_frame_state.composite_fence]).unwrap();
    }
    queue_command_buffer_submit(
        state.submit_sender.as_ref().unwrap(),
        state.submit_mutex.as_ref(),
        cur_frame_state.command_buf.clone().unwrap(),
        state.swapchain.as_ref().unwrap(),
        device.queues.graphics_family,
        vec![cur_frame_state.rebuild_semaphore],
        vec![vk::PipelineStageFlags::ALL_COMMANDS],
        vec![cur_frame_state.draw_semaphore],
        Some(cur_frame_state.composite_fence),
        None,
    )
        .unwrap();
}

pub(crate) fn draw_framebuffer_to_swapchain(
    device: &VulkanDevice,
    frame_state: &mut PerFrameData,
    viewport: &Viewport,
    swapchain: &VulkanSwapchain,
    pipeline: &PipelineInfo,
    cmd_buf: CommandBufferInfo,
) {
    let resolution = swapchain.resolution;

    let viewport_px = transform_viewport_to_pixels(viewport, &resolution);

    let cur_desc_set = frame_state.composite_desc_sets[0];
    let cur_desc_sets = [cur_desc_set];

    let fb_width = swapchain.resolution.x;
    let fb_height = swapchain.resolution.y;

    let vk_cmd_buf = unsafe { cmd_buf.get_handle() };

    let vk_viewport = vk::Viewport::default()
        .width(fb_width as f32)
        .height(fb_height as f32)
        .x(-(viewport_px.left as f32))
        .y(-(viewport_px.top as f32));
    unsafe {
        device.logical_device.cmd_set_viewport(vk_cmd_buf, 0, &[vk_viewport]);
    }

    let scissor = vk::Rect2D::default()
        .extent(vk::Extent2D {
            width: fb_width,
            height: fb_height,
        })
        .offset(vk::Offset2D {
            x: 0,
            y: 0,
        });
    unsafe {
        device.logical_device.cmd_set_scissor(vk_cmd_buf, 0, &[scissor]);
    }

    unsafe {
        device.logical_device.cmd_bind_descriptor_sets(
            vk_cmd_buf,
            vk::PipelineBindPoint::GRAPHICS,
            pipeline.layout,
            0,
            &cur_desc_sets,
            &[],
        );
    }

    unsafe {
        device.logical_device.cmd_draw(vk_cmd_buf, 6, 1, 0, 0);
    }
}
