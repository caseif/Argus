use crate::renderer::bucket_proc::fill_buckets;
use crate::renderer::compositing::{draw_framebuffer_to_swapchain, draw_scene_to_framebuffer};
use crate::renderer::scene_compiler::compile_scene_2d;
use crate::LOGGER;
use crate::state::{RendererState, Scene2dState, ViewportState};
use crate::defines::*;
use argus_logging::debug;
use argus_render::common::{AttachedViewport, Material, RenderCanvas, SceneType, TextureData};
use argus_render::constants::{SHADER_UBO_GLOBAL_LEN, SHADER_UBO_SCENE_LEN};
use argus_render::twod::{get_render_context_2d, ViewportYAxisConvention};
use argus_resman::{ResourceIdentifier, ResourceManager};
use argus_util::math::Vector2u;
use argus_util::semaphore::Semaphore;
use argus_wm::{vk_create_surface, VkInstance, Window};
use std::collections::HashSet;
use std::sync::atomic::Ordering;
use std::sync::mpsc;
use std::time::Duration;
use vk_wrapper::*;
use crate::renderer::pipelines::create_pipeline_for_shaders;

const FRAME_QUAD_VERTEX_DATA: &[f32] = &[
    -1.0, -1.0, 0.0, 0.0,
    -1.0, 1.0, 0.0, 1.0,
    1.0, 1.0, 1.0, 1.0,
    -1.0, -1.0, 0.0, 0.0,
    1.0, 1.0, 1.0, 1.0,
    1.0, -1.0, 1.0, 0.0,
];

pub(crate) struct VulkanRenderer<'dev, 'inst: 'dev> {
    vk_instance: &'inst vk::Instance,
    vk_device: &'dev vk::Device<'inst>,
    state: RendererState<'dev>,
    is_initted: bool,
}

impl<'dev, 'inst> VulkanRenderer<'dev, 'inst> {
    pub(crate) fn new(
        vk_instance: &'inst vk::Instance,
        vk_device: &'dev vk::Device<'inst>,
        window: &Window,
    )
        -> Self {
        let mut renderer = Self {
            vk_instance,
            vk_device,
            state: RendererState::new(vk_device),
            is_initted: false,
        };
        renderer.init(window);
        renderer
    }

    pub(crate) fn is_initialized(&self) -> bool {
        self.is_initted
    }

    pub(crate) fn init(&mut self, window: &Window) {
        let surface = {
            // SAFETY: `self` can only be created via ::new which accepts a
            // Vulkan instance via a safe vk::Instance object, and the returned
            // wm object only lives to the end of this scope and thus can only
            // be used by the `vk_create_surface` call and nothing else.
            let wm_instance = unsafe { VkInstance::from_raw(self.vk_instance.get_handle()) };

            let wm_surface = vk_create_surface(window, &wm_instance)
                .expect("Failed to create Vulkan surface");

            // SAFETY: We just created the surface handle, and it is consumed by the
            // wrapper object thus guaranteeing all future interaction will be done
            // through the wrapper.
            unsafe { vk::Surface::from_handle(self.vk_instance, wm_surface.into_raw()) }
        };
        debug!(LOGGER, "Created surface for new window");

        self.state.graphics_command_pool = Some(vk::CommandPool::create(
            self.vk_device,
            self.vk_device.queue_indices.graphics_family,
        ));
        debug!(LOGGER, "Created command pools for new window");
        let gfx_command_pool = self.state.graphics_command_pool.as_ref().unwrap();

        self.state.desc_pool = Some(vk::DescriptorPool::create(&self.vk_device).unwrap());
        debug!(LOGGER, "Created descriptor pool for new window");

        gfx_command_pool.alloc_buffers(vk::MAX_FRAMES_IN_FLIGHT as u32).into_iter().enumerate()
            .for_each(|(i, buf)| {
                self.state.copy_cmd_buf[i] = Some(buf);
            });
        debug!(LOGGER, "Created command buffers for new window");

        /*VkSemaphoreCreateInfo sem_info{};
        sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(state.device.logical_device, &sem_info, None, &state.rebuild_semaphore)
                != VK_SUCCESS) {
            panic!("Failed to create semaphores for new window");
        }
        debug!(LOGGER, "Created semaphores for new window");*/

        self.state.global_ubo = Some(vk::Buffer::new(
            &self.vk_device,
            SHADER_UBO_GLOBAL_LEN as vk::DeviceSize,
            vk::BufferUsageFlags::UNIFORM_BUFFER,
            vk::MEM_CLASS_DEVICE_RW,
        ).unwrap());

        self.state.swapchain = Some(vk::Swapchain::create(
            &self.vk_device,
            surface,
            window.peek_resolution(),
            SHADER_OUT_COLOR_LOC,
        ).unwrap());
        debug!(LOGGER, "Created swapchain for new window");
        let swapchain = self.state.swapchain.as_ref().unwrap();

        let (submit_tx, submit_rx) = mpsc::channel();
        let present_queue_mutex_opt =
            if self.vk_device.queues.present_family != self.vk_device.queues.graphics_family {
                Some(self.vk_device.queue_mutexes.present_family.clone())
            } else {
                None
            };

        let submit_thread_handle = vk::start_submit_queues_thread(
            self.vk_device,
            swapchain,
            self.state.submit_mutex.clone(),
            &self.vk_device.queues.graphics_family,
            self.vk_device.queue_mutexes.graphics_family.clone(),
            present_queue_mutex_opt,
            submit_rx,
        );
        self.state.submit_thread = Some(submit_thread_handle);
        self.state.submit_sender = Some(submit_tx);

        self.state.composite_pipeline = Some(create_pipeline_for_shaders(
            &self.vk_device,
            &[&FB_SHADER_VERT_PATH, &FB_SHADER_FRAG_PATH],
            &self.state.viewport_size,
            &swapchain.composite_render_pass,
        ).unwrap());
        debug!(LOGGER, "Created composite pipeline");

        let mut composite_vbo = vk::Buffer::new(
            &self.vk_device,
            size_of_val(FRAME_QUAD_VERTEX_DATA) as vk::DeviceSize,
            vk::BufferUsageFlags::VERTEX_BUFFER,
            vk::MEM_CLASS_DEVICE_RW,
        ).unwrap();
        {
            let mut composite_vbo_mapped = composite_vbo.map(
                &self.vk_device,
                0,
                size_of_val(FRAME_QUAD_VERTEX_DATA) as vk::DeviceSize,
                vk::MemoryMapFlags::empty(),
            ).unwrap();
            composite_vbo_mapped.as_slice_mut().copy_from_slice(FRAME_QUAD_VERTEX_DATA);
        }
        self.state.composite_vbo = Some(composite_vbo);
        debug!(LOGGER, "Created composite VBO");

        self.state.fb_render_pass = Some(vk::RenderPass::create_basic(
            &self.vk_device,
            swapchain.image_format,
            vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
            SHADER_OUT_COLOR_LOC,
            Some(SHADER_OUT_LIGHT_OPACITY_LOC),
        ).unwrap());
        debug!(LOGGER, "Created framebuffer render pass for new window");

        for i in 0..vk::MAX_FRAMES_IN_FLIGHT {
            self.state.present_submitted_sem[i].notify();
            self.state.command_buffer_submitted_sem[i].notify();
        }

        self.is_initted = true;
    }

    pub(crate) fn destroy(mut self) {
        let device: &vk::Device = &self.vk_device;

        if !self.is_initted {
            return;
        }

        self.state.submit_halt = true;
        if let Some(submit_thread) = self.state.submit_thread.take() {
            self.state.submit_sender.unwrap().send(
                vk::SubmitMessage::NotifyHalting(
                    vk::NotifyHaltingParams {
                        ack_sem: self.state.submit_halt_acked.clone()
                    }
                )
            ).unwrap();
            self.state.submit_halt_acked.wait();
            submit_thread.join().unwrap();
        }

        {
            for sem in &self.state.present_submitted_sem {
                sem.wait();
            }
            let _queue_lock = self.vk_device.queue_mutexes.graphics_family.lock().unwrap();
            device.queues.graphics_family.wait_idle().unwrap();
        }

        for (_, viewport_state) in self.state.viewport_states_2d.drain() {
            viewport_state.destroy(
                self.state.desc_pool.as_ref().unwrap(),
                self.state.graphics_command_pool.as_ref().unwrap(),
            );
        }

        for (_, scene_state) in self.state.scene_states_2d.drain() {
            scene_state.destroy();
        }

        for cb in self.state.copy_cmd_buf.into_iter().flatten() {
            cb.destroy(self.state.graphics_command_pool.as_ref().unwrap());
        }

        for (_, (comp_cmd_buf, _)) in self.state.composite_cmd_bufs {
            comp_cmd_buf.destroy(self.state.graphics_command_pool.as_ref().unwrap());
        }

        if let Some(pool) = self.state.desc_pool {
            pool.destroy();
        }

        if let Some(vbo) = self.state.composite_vbo {
            vbo.destroy();
        }

        if let Some(pipeline) = self.state.composite_pipeline {
            pipeline.destroy();
        }

        for (_, pipeline) in self.state.material_pipelines {
            pipeline.destroy();
        }

        if let Some(pass) = self.state.fb_render_pass {
            pass.destroy();
        }

        if let Some(pool) = self.state.graphics_command_pool {
            pool.destroy();
        }

        for (_, texture) in self.state.prepared_textures {
            texture.destroy();
        }

        if let Some(swapchain) = self.state.swapchain {
            let surface = swapchain.destroy().unwrap();
            surface.destroy();
        }

        if let Some(ubo) = self.state.global_ubo {
            ubo.destroy();
        }
    }

    pub(crate) fn render(&mut self, window: &mut Window, _delta: Duration) {
        let cur_frame = self.state.cur_frame.load(Ordering::Acquire);

        /*static auto last_print = std::chrono::high_resolution_clock::now();
        static int64_t time_samples = 0;

        static std::chrono::nanoseconds compile_time;
        static std::chrono::nanoseconds rebuild_time;
        static std::chrono::nanoseconds draw_time;
        static std::chrono::nanoseconds composite_time;

        std::chrono::high_resolution_clock::time_point timer_start;
        std::chrono::high_resolution_clock::time_point timer_end;

        if std::chrono::high_resolution_clock::now() - last_print >= 10s && time_samples > 0 {
            debug!(LOGGER, "Compile + rebuild + draw + composite took %ld + %ld + %ld + %ld ns\n",
                    compile_time.count() / time_samples,
                    rebuild_time.count() / time_samples,
                    draw_time.count() / time_samples,
                    composite_time.count() / time_samples);

            compile_time = rebuild_time = draw_time = composite_time = 0ns;
            time_samples = 0;
            last_print = std::chrono::high_resolution_clock::now();
        }*/

        let vsync = window.is_vsync_enabled();
        if vsync.dirty {
            //glfwSwapInterval(vsync ? 1 : 0);
        }

        add_remove_state_objects(window, &mut self.state);

        let resolution = window.get_resolution();

        if !self.state.are_viewports_initialized {
            self.update_view_states(window, &resolution.value, true);
            self.state.are_viewports_initialized = true;
        }

        self.update_view_states(window, &resolution.value, false);

        //timer_start = std::chrono::high_resolution_clock::now();
        compile_scenes(window, &mut self.state);
        //timer_end = std::chrono::high_resolution_clock::now();
        //compile_time += (timer_end - timer_start);

        {
            //state.present_sem[state.cur_frame].wait();
        }

        let sc_image_index = get_next_image(&self.vk_device, &mut self.state, cur_frame);

        //timer_start = std::chrono::high_resolution_clock::now();
        record_scene_rebuild(
            window,
            &mut self.state,
            cur_frame,
        );
        submit_scene_rebuild(&self.vk_device, &mut self.state, cur_frame);
        //timer_end = std::chrono::high_resolution_clock::now();
        //rebuild_time += (timer_end - timer_start);

        let canvas = window.get_canvas_mut().unwrap()
            .as_any_mut().downcast_mut::<RenderCanvas>().unwrap();
        let viewport_ids = canvas.get_viewports_2d();

        //timer_start = std::chrono::high_resolution_clock::now();
        for viewport_id in viewport_ids {
            draw_scene_to_framebuffer(
                &mut self.state,
                viewport_id,
                resolution,
                cur_frame,
            );
        }
        let viewports = canvas.get_viewports_2d();

        //timer_end = std::chrono::high_resolution_clock::now();
        //draw_time += (timer_end - timer_start);

        // set up state for drawing framebuffers to screen

        //glClearColor(0.0, 0.0, 0.0, 1.0);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //timer_start = std::chrono::high_resolution_clock::now();

        if self.state.dirty_viewports || resolution.dirty {
            for (_, flag) in self.state.composite_cmd_bufs.values_mut() {
                *flag = true;
            }
            self.state.dirty_viewports = false;
        }

        composite_framebuffers(
            &mut self.state,
            &viewports,
            sc_image_index,
            cur_frame,
        );

        submit_composite(&self.vk_device, &mut self.state, sc_image_index, cur_frame);
        //timer_end = std::chrono::high_resolution_clock::now();
        //composite_time += (timer_end - timer_start);

        present_image(&mut self.state, sc_image_index, cur_frame);

        if self.state.cur_frame.compare_exchange(
            vk::MAX_FRAMES_IN_FLIGHT - 1,
            0,
            Ordering::Relaxed,
            Ordering::Relaxed,
        )
            .is_err() {
            self.state.cur_frame.fetch_add(1, Ordering::Relaxed);
        }

        //time_samples++;
    }

    pub(crate) fn notify_window_resize(&mut self, window: &mut Window, resolution: Vector2u) {
        self.update_view_states(window, &resolution, true);

        let submit_ack_sem = Semaphore::default();
        self.state.submit_sender.as_ref().unwrap().send(
            vk::SubmitMessage::NotifyDestroyedSwapchain(vk::NotifyDestroyedSwapchainParams::new(
                self.state.swapchain.as_ref().unwrap(),
                submit_ack_sem.clone(),
            ))
        ).unwrap();
        submit_ack_sem.wait();

        let _submit_lock = self.state.submit_mutex.lock().unwrap();
        let _gfx_queue_lock = self.vk_device.queue_mutexes.graphics_family.lock().unwrap();
        let _present_queue_lock =
            if self.vk_device.queues.present_family != self.vk_device.queues.graphics_family {
                Some(self.vk_device.queue_mutexes.present_family.lock().unwrap())
            } else {
                None
            };

        self.state.swapchain = Some(
            self.state.swapchain.take().unwrap().recreate(
                resolution,
                _submit_lock,
                _gfx_queue_lock,
                _present_queue_lock,
            ).unwrap()
        );
        self.state.submit_sender.as_ref().unwrap().send(vk::SubmitMessage::NotifyCreatedSwapchain(
            vk::NotifyCreatedSwapchainParams::new(self.state.swapchain.as_ref().unwrap())
        )).unwrap();
    }

    fn update_view_states(&mut self, window: &mut Window, resolution: &Vector2u, force: bool) {
        let canvas = window.get_canvas_mut().unwrap().as_any_mut().downcast_mut::<RenderCanvas>().unwrap();

        for viewport_id in canvas.get_viewports_2d() {
            let mut viewport = get_render_context_2d().get_viewport_mut(viewport_id)
                .expect("Viewport was missing from context!");

            let camera_transform = {
                let mut scene = get_render_context_2d().get_scene_mut(viewport.get_scene_id()).unwrap();
                let camera = scene.get_camera_mut(viewport.get_camera_id()).unwrap();
                camera.get_transform()
            };

            if force || camera_transform.dirty {
                viewport.update_view_state(&resolution, ViewportYAxisConvention::TopDown);

                if let Some(viewport_state) = self.state.viewport_states_2d
                    .get_mut(&viewport.get_id()) {
                    for per_frame in &mut viewport_state.per_frame {
                        per_frame.view_matrix_dirty = true;
                    }
                }
            }
        }
    }
}

fn get_associated_scenes_for_canvas(canvas: &RenderCanvas) -> HashSet<String> {
    canvas.get_viewports_2d().iter()
        .map(|&id| get_render_context_2d().get_viewport(id).unwrap().get_scene_id().to_string())
        .collect()
}

fn create_viewport_2d_state<'ctx>(
    device: &'ctx vk::Device<'ctx>,
    command_pool: &vk::CommandPool<'ctx>,
    viewport_id: u32,
) -> ViewportState<'ctx> {
    let mut viewport_state = ViewportState::new(viewport_id);

    let sem_info = vk::SemaphoreCreateInfo::default();

    for frame_state in &mut viewport_state.per_frame {
        frame_state.rebuild_semaphore = Some(vk::Semaphore::create(device, &sem_info)
            .expect("Failed to create semaphores for viewport"));

        frame_state.draw_semaphore = Some(vk::Semaphore::create(device, &sem_info)
            .expect("Failed to create semaphores for viewport"));

        let fence_info = vk::FenceCreateInfo::default()
            .flags(vk::FenceCreateFlags::empty());
        frame_state.composite_fence = Some(vk::Fence::create(device, &fence_info)
            .expect("Failed to create fences for viewport"));

        frame_state.command_buf = Some(command_pool.alloc_buffers(1).into_iter().next().unwrap());

        frame_state.scene_ubo = Some(vk::Buffer::new(
            device,
            SHADER_UBO_SCENE_LEN as vk::DeviceSize,
            vk::BufferUsageFlags::UNIFORM_BUFFER,
            vk::MEM_CLASS_HOST_RW,
        ).unwrap());
    }

    viewport_state
}

fn create_scene_state<'ctx>(
    device: &'ctx vk::Device,
    scene_id: impl Into<String>,
) -> Scene2dState<'ctx> {
    let mut scene_state = Scene2dState::new(scene_id.into());

    scene_state.ubo = Some(vk::Buffer::new(
        device,
        SHADER_UBO_SCENE_LEN as vk::DeviceSize,
        vk::BufferUsageFlags::UNIFORM_BUFFER,
        vk::MEM_CLASS_HOST_RW,
    ).unwrap());

    scene_state
}

fn add_remove_state_objects<'ctx>(
    window: &Window,
    state: &mut RendererState<'ctx>,
) {
    let canvas = window.get_canvas().unwrap().as_any().downcast_ref::<RenderCanvas>().unwrap();

    for viewport_id in canvas.get_viewports_2d() {
        let vp_state = state.viewport_states_2d.entry(viewport_id)
            .or_insert_with(|| create_viewport_2d_state(
                state.device,
                state.graphics_command_pool.as_ref().unwrap(),
                viewport_id,
            ));

        vp_state.visited = true;

        let scene_id = {
            get_render_context_2d().get_viewport(viewport_id)
                .expect("Viewport was missing from context!")
                .get_scene_id().to_string()
        };
        let scene_state = state.scene_states_2d.entry(scene_id.to_owned())
            .or_insert_with(|| create_scene_state(state.device, scene_id));
        scene_state.visited = true;
    }

    // destroy scene states which were not visited this frame
    let stale_scene_states = state.scene_states_2d
        .extract_if(|_, scene_state| !scene_state.visited);
    for (_, scene_state) in stale_scene_states {
        scene_state.destroy();
    }
    // clear visited flag for all remaining states
    for scene_state in state.scene_states_2d.values_mut() {
        scene_state.visited = false;
    }

    // destroy scene states which were not visited last frame
    let stale_viewport_states = state.viewport_states_2d
        .extract_if(|_, viewport_state| !viewport_state.visited);
    for (_, viewport_state) in stale_viewport_states {
        viewport_state.destroy(
            state.desc_pool.as_ref().unwrap(),
            state.graphics_command_pool.as_ref().unwrap(),
        );

        state.dirty_viewports = true;
    }
    // clear visited flag for all remaining states
    for viewport_state in state.viewport_states_2d.values_mut() {
        viewport_state.visited = false;
    }
}

fn compile_scenes(
    window: &Window,
    state: &mut RendererState,
) {
    let canvas = window.get_canvas().unwrap().as_any().downcast_ref::<RenderCanvas>().unwrap();

    for scene_id in get_associated_scenes_for_canvas(canvas) {
        compile_scene_2d(state, &scene_id);
    }
}

fn check_scene_ubo_dirty(state: &mut RendererState, scene_id: &str) {
    let scene_state = state.scene_states_2d.get(scene_id).unwrap();
    if scene_state.scene_type == SceneType::TwoDim {
        let mut scene = get_render_context_2d().get_scene_mut(&scene_state.scene_id).unwrap();

        let al_level = scene.get_ambient_light_level();
        let al_color = scene.get_ambient_light_color();

        let must_update = al_level.dirty || al_color.dirty;

        if must_update {
            /*auto &staging_ubo = scene_state.scene_ubo_staging;

            if al_color.dirty {
                float al_color_arr[3] = { al_color.r, al_color.g, al_color.b };
                write_to_buffer(staging_ubo, al_color_arr, SHADER_UNIFORM_SCENE_AL_COLOR_OFF, sizeof(al_color_arr));
            }

            if al_level.dirty {
                write_val_to_buffer(staging_ubo, al_level.value, SHADER_UNIFORM_SCENE_AL_LEVEL_OFF);
            }*/

            for viewport_state in state.viewport_states_2d.values_mut() {
                for per_frame in &mut viewport_state.per_frame {
                    per_frame.scene_ubo_dirty = true;
                }
            }
        }
    }
}

fn record_scene_rebuild(
    window: &Window,
    state: &mut RendererState,
    cur_frame: usize,
) {
    let device = state.device;

    let canvas = window.get_canvas().unwrap().as_any().downcast_ref::<RenderCanvas>().unwrap();

    state.copy_cmd_buf[cur_frame].as_ref().unwrap().begin_oneshot_commands();

    for scene_id in get_associated_scenes_for_canvas(canvas) {
        check_scene_ubo_dirty(state, &scene_id);

        let scene_state = state.scene_states_2d.get_mut(&scene_id).unwrap();

        fill_buckets(
            device,
            scene_state,
            state.copy_cmd_buf[cur_frame].as_ref().unwrap(),
            &state.material_pipelines,
        );

        for bucket in scene_state.render_buckets.values_mut() {
            let material_res = bucket.material_res.clone();

            let copy_cmd_buf = state.copy_cmd_buf[cur_frame].as_ref()
                .expect("Copy command buffer is not initialized");

            let texture_uid = ResourceIdentifier::parse(
                material_res.get::<Material>().unwrap().get_texture_uid()
            ).unwrap();

            if state.prepared_textures.get(&texture_uid).is_some() {
                state.material_textures.insert(
                    material_res.get_prototype().uid.clone(),
                    texture_uid.clone(),
                );
                continue;
            }

            let texture_res = ResourceManager::instance().get_resource(texture_uid.to_string())
                .expect("Failed to load texture"); //TODO

            let texture_obj = texture_res.get::<TextureData>().unwrap();
            let prepared = vk::prepare_texture(
                device,
                copy_cmd_buf,
                texture_obj.get_width(),
                texture_obj.get_height(),
                texture_obj.get_pixel_data(),
            )
                .expect("Failed to prepare texture");

            state.prepared_textures.insert(texture_uid.clone(), prepared);
            state.material_textures.insert(
                material_res.get_prototype().uid.clone(),
                texture_uid.clone(),
            );
        }
    }

    state.copy_cmd_buf[cur_frame].as_ref().unwrap().end_commands();
}

fn submit_scene_rebuild(device: &vk::Device, state: &mut RendererState, cur_frame: usize) {
    let rebuild_sems = state.viewport_states_2d.values()
        .filter_map(|vp_state| vp_state.per_frame[cur_frame].rebuild_semaphore)
        .collect();
    state.copy_cmd_buf[cur_frame].as_ref().unwrap().queue_submit(
        state.submit_sender.as_ref().unwrap(),
        state.submit_mutex.as_ref(),
        state.swapchain.as_ref().unwrap(),
        &device.queues.graphics_family,
        vec![],
        vec![],
        rebuild_sems,
        None,
        None,
    )
        .unwrap();

    /*for buf in state.texture_bufs_to_free {
        free_buffer(buf);
    }
    state.texture_bufs_to_free.clear();*/
}

fn get_next_image(device: &vk::Device, state: &mut RendererState, cur_frame: usize) -> u32 {
    let swapchain = &state.swapchain.as_ref().expect("Swapchain is not initialized");

    state.command_buffer_submitted_sem[cur_frame].wait();
    device.wait_for_fences(
        &[&swapchain.in_flight_fence[cur_frame]],
        true,
        u64::MAX,
    )
        .expect("vkWaitForFences failed");
    device.reset_fences(
        &[&swapchain.in_flight_fence[cur_frame]],
    )
        .expect("vkResetFences failed");

    let _submit_guard = state.submit_mutex.lock().unwrap();

    let (image_index, _) = swapchain.acquire_next_image(cur_frame, u64::MAX)
        .expect("vkAcquireNextImageKHR failed");

    image_index
}

fn composite_framebuffers<'ctx>(
    state: &mut RendererState<'ctx>,
    viewports: &Vec<u32>,
    sc_image_index: u32,
    cur_frame: usize,
) {
    let cmd_buf = if let Some((buf, flag)) = state.composite_cmd_bufs.get_mut(&sc_image_index) {
        if !*flag {
            return;
        }

        *flag = false;

        buf
    } else {
        let new_cmd_buf =
            state.graphics_command_pool.as_ref().unwrap()
                .alloc_buffers(1).into_iter().next().unwrap();
        state.composite_cmd_bufs.insert(sc_image_index, (new_cmd_buf, false));
        &state.composite_cmd_bufs.get(&sc_image_index).unwrap().0
    };

    cmd_buf.reset(vk::CommandBufferResetFlags::empty());

    let cmd_begin_info = vk::CommandBufferBeginInfo::default()
        .flags(vk::CommandBufferUsageFlags::empty());
    cmd_buf.begin_commands(&cmd_begin_info).unwrap();

    let swapchain = state.swapchain.as_ref().expect("Swapchain is not initialized");

    let fb_width = swapchain.extent.width;
    let fb_height = swapchain.extent.height;

    let clear_val = vk::ClearValue {
        color: vk::ClearColorValue { float32: [0.0, 0.0, 0.0, 1.0] },
    };
    let clear_vals = [clear_val];

    let rp_info = vk::RenderPassBeginInfo::default()
        .framebuffer(&swapchain.framebuffers[sc_image_index as usize])
        .clear_values(&clear_vals)
        .render_pass(&swapchain.composite_render_pass)
        .render_area(vk::Rect2D {
            extent: vk::Extent2D { width: fb_width, height: fb_height },
            offset: vk::Offset2D { x: 0, y: 0 },
        });
    cmd_buf.cmd_begin_render_pass(&rp_info, vk::SubpassContents::INLINE);

    cmd_buf.cmd_bind_pipeline(
        vk::PipelineBindPoint::GRAPHICS,
        state.composite_pipeline.as_ref().unwrap(),
    );

    cmd_buf.cmd_bind_vertex_buffers(
        0,
        &[state.composite_vbo.as_ref().unwrap()],
        &[0],
    );

    for &viewport_id in viewports {
        let viewport_state = state.viewport_states_2d.get_mut(&viewport_id).unwrap();
        let viewport = get_render_context_2d().get_viewport(viewport_id)
            .expect("Viewport was missing from context!")
            .get_viewport().clone();

        draw_framebuffer_to_swapchain(
            &mut viewport_state.per_frame[cur_frame],
            &viewport,
            state.swapchain.as_ref().unwrap(),
            state.composite_pipeline.as_ref().unwrap(),
            cmd_buf,
        );
    }

    cmd_buf.cmd_end_render_pass();

    cmd_buf.end_commands();
}

fn submit_composite(
    device: &vk::Device,
    state: &mut RendererState,
    sc_image_index: u32,
    cur_frame: usize,
) {
    let swapchain = state.swapchain.as_ref().expect("Swapchain is not initialized");

    let mut wait_sems = Vec::with_capacity(state.viewport_states_2d.len() + 1);
    let mut wait_stages = Vec::with_capacity(state.viewport_states_2d.len() + 1);
    wait_sems.push(swapchain.image_avail_sem[cur_frame]);
    wait_stages.push(vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT);
    for viewport_state in state.viewport_states_2d.values() {
        if let Some(draw_sem) = viewport_state.per_frame[cur_frame].draw_semaphore {
            wait_sems.push(draw_sem);
            wait_stages.push(vk::PipelineStageFlags::ALL_COMMANDS);
        }
    }

    state.composite_cmd_bufs.get(&sc_image_index).unwrap().0.queue_submit(
        state.submit_sender.as_ref().unwrap(),
        state.submit_mutex.as_ref(),
        state.swapchain.as_ref().unwrap(),
        &device.queues.graphics_family,
        wait_sems,
        wait_stages,
        vec![swapchain.render_done_sem[sc_image_index as usize]],
        Some(swapchain.in_flight_fence[cur_frame]),
        Some(state.command_buffer_submitted_sem[cur_frame].clone()),
    )
        .unwrap();
}

fn present_image(state: &mut RendererState, image_index: u32, cur_frame: usize) {
    let swapchain = state.swapchain.as_ref().unwrap();
    state.submit_sender.as_ref().unwrap()
        .send(vk::SubmitMessage::PresentImage(vk::PresentImageParams::new(
            swapchain,
            vec![
                swapchain.render_done_sem[image_index as usize],
            ],
            image_index,
            state.present_submitted_sem[cur_frame].clone(),
        )))
        .unwrap();
}
