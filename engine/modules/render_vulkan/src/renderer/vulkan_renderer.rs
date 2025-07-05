use crate::renderer::bucket_proc::fill_buckets;
use crate::renderer::compositing::{draw_framebuffer_to_swapchain, draw_scene_to_framebuffer};
use crate::renderer::scene_compiler::compile_scene_2d;
use crate::setup::device::VulkanDevice;
use crate::setup::instance::VulkanInstance;
use crate::setup::swapchain::{create_swapchain, destroy_swapchain, recreate_swapchain, VulkanSwapchain};
use crate::setup::LOGGER;
use crate::state::{NotifyCreatedSwapchainParams, NotifyDestroyedSwapchainParams, NotifyHaltingParams, PresentImageParams, RendererState, Scene2dState, SubmitMessage, ViewportState};
use crate::util::defines::*;
use crate::util::*;
use argus_core::ScreenSpaceScaleMode;
use argus_logging::debug;
use argus_render::common::{AttachedViewport, Material, Matrix4x4, RenderCanvas, SceneType, Transform2d, Viewport};
use argus_render::constants::{SHADER_UBO_GLOBAL_LEN, SHADER_UBO_SCENE_LEN};
use argus_render::twod::{get_render_context_2d, AttachedViewport2d};
use argus_resman::{ResourceIdentifier, ResourceManager};
use argus_util::math::Vector2u;
use argus_util::semaphore::Semaphore;
use argus_wm::{vk_create_surface, VkInstance, Window};
use ash::vk::Handle;
use ash::{khr, vk};
use std::collections::HashSet;
use std::ops::Deref;
use std::sync::atomic::Ordering;
use std::sync::{mpsc, Arc, Mutex};
use std::thread;
use std::thread::JoinHandle;
use std::time::Duration;

const FRAME_QUAD_VERTEX_DATA: &[f32] = &[
    -1.0, -1.0, 0.0, 0.0,
    -1.0, 1.0, 0.0, 1.0,
    1.0, 1.0, 1.0, 1.0,
    -1.0, -1.0, 0.0, 0.0,
    1.0, 1.0, 1.0, 1.0,
    1.0, -1.0, 1.0, 0.0,
];

pub(crate) struct VulkanRenderer {
    vk_instance: VulkanInstance,
    vk_device: VulkanDevice,
    state: RendererState,
    is_initted: bool,
}

impl VulkanRenderer {
    pub(crate) fn new(vk_instance: VulkanInstance, vk_device: VulkanDevice, window: &Window) -> Self {
        let mut renderer = Self {
            vk_instance,
            vk_device,
            state: RendererState::default(),
            is_initted: false,
        };
        renderer.init(window);
        renderer
    }

    pub(crate) fn is_initialized(&self) -> bool {
        self.is_initted
    }

    pub(crate) fn init(&mut self, window: &Window) {
        let ash_instance = self.vk_instance.get_underlying().handle();
        let wm_instance = VkInstance::from_raw(ash_instance.as_raw());
        let wm_surface = vk_create_surface(window, &wm_instance)
            .expect("Failed to create Vulkan surface");
        let ash_surface = vk::SurfaceKHR::from_raw(wm_surface.as_raw());
        self.state.surface = Some(ash_surface);
        let surface = self.state.surface.as_ref().unwrap();
        debug!(LOGGER, "Created surface for new window");

        self.state.graphics_command_pool = Some(create_command_pool(
            &self.vk_device,
            self.vk_device.queue_indices.graphics_family,
        ));
        debug!(LOGGER, "Created command pools for new window");
        let gfx_command_pool = self.state.graphics_command_pool.as_ref().unwrap();

        self.state.desc_pool = Some(create_descriptor_pool(&self.vk_device).unwrap());
        debug!(LOGGER, "Created descriptor pool for new window");

        let copy_cmd_bufs = alloc_command_buffers(
            &self.vk_device,
            *gfx_command_pool,
            MAX_FRAMES_IN_FLIGHT as u32,
        );
        #[allow(clippy::needless_range_loop)]
        for i in 0..MAX_FRAMES_IN_FLIGHT {
            self.state.copy_cmd_buf[i] = Some(copy_cmd_bufs[i].clone());
        }
        debug!(LOGGER, "Created command buffers for new window");

        /*VkSemaphoreCreateInfo sem_info{};
        sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(state.device.logical_device, &sem_info, None, &state.rebuild_semaphore)
                != VK_SUCCESS) {
            panic!("Failed to create semaphores for new window");
        }
        debug!(LOGGER, "Created semaphores for new window");*/

        self.state.global_ubo = Some(VulkanBuffer::new(
            &self.vk_instance,
            &self.vk_device,
            SHADER_UBO_GLOBAL_LEN as vk::DeviceSize,
            vk::BufferUsageFlags::UNIFORM_BUFFER,
            MEM_CLASS_DEVICE_RW,
        ).unwrap());

        self.state.swapchain = Some(create_swapchain(
            &self.vk_instance,
            &self.vk_device,
            *surface,
            window.peek_resolution(),
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

        let submit_thread_handle = start_submit_queues_thread(
            self.vk_device.logical_device.clone(),
            self.vk_device.ext_khr_swapchain.clone(),
            swapchain,
            self.state.submit_mutex.clone(),
            self.vk_device.queues.graphics_family,
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
            swapchain.composite_render_pass,
        ).unwrap());
        debug!(LOGGER, "Created composite pipeline");

        let mut composite_vbo = VulkanBuffer::new(
            &self.vk_instance,
            &self.vk_device,
            size_of_val(FRAME_QUAD_VERTEX_DATA) as vk::DeviceSize,
            vk::BufferUsageFlags::VERTEX_BUFFER,
            MEM_CLASS_DEVICE_RW,
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

        self.state.fb_render_pass = Some(create_render_pass(
            &self.vk_device,
            swapchain.image_format,
            vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
            true,
        ).unwrap());
        debug!(LOGGER, "Created framebuffer render pass for new window");

        for i in 0..MAX_FRAMES_IN_FLIGHT {
            self.state.present_submitted_sem[i].notify();
            self.state.command_buffer_submitted_sem[i].notify();
        }

        self.is_initted = true;
    }

    pub(crate) fn destroy(mut self) {
        let device: &VulkanDevice = &self.vk_device;

        if !self.is_initted {
            return;
        }

        self.state.submit_halt = true;
        if let Some(submit_thread) = self.state.submit_thread.take() {
            self.state.submit_sender.unwrap().send(
                SubmitMessage::NotifyHalting(
                    NotifyHaltingParams {
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
            unsafe {
                device.logical_device.queue_wait_idle(device.queues.graphics_family).unwrap();
            }
        }

        for (_, viewport_state) in self.state.viewport_states_2d.drain() {
            destroy_viewport(&self.vk_device, self.state.desc_pool.unwrap(), viewport_state);
        }

        for (_, scene_state) in self.state.scene_states_2d.drain() {
            destroy_scene(&self.vk_device, scene_state);
        }

        for cb in self.state.copy_cmd_buf.into_iter().flatten() {
            destroy_command_buffer(device, cb);
        }

        for (_, (comp_cmd_buf, _)) in self.state.composite_cmd_bufs {
            destroy_command_buffer(device, comp_cmd_buf);
        }

        if let Some(pool) = self.state.desc_pool {
            destroy_descriptor_pool(device, pool);
        }

        if let Some(vbo) = self.state.composite_vbo {
            vbo.destroy(device);
        }

        if let Some(pipeline) = self.state.composite_pipeline {
            destroy_pipeline(device, pipeline);
        }

        for (_, pipeline) in self.state.material_pipelines {
            destroy_pipeline(device, pipeline);
        }

        if let Some(pass) = self.state.fb_render_pass {
            destroy_render_pass(device, pass);
        }

        if let Some(pool) = self.state.graphics_command_pool {
            destroy_command_pool(device, pool);
        }

        for (_, texture) in self.state.prepared_textures {
            destroy_texture(device, texture);
        }

        if let Some(swapchain) = self.state.swapchain {
            unsafe {
                destroy_swapchain(device, swapchain).unwrap();
            }
        }

        if let Some(ubo) = self.state.global_ubo {
            ubo.destroy(device);
        }

        if let Some(surface) = self.state.surface {
            unsafe {
                self.vk_instance.khr_surface().destroy_surface(surface, None);
            }
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

        add_remove_state_objects(&self.vk_instance, &self.vk_device, window, &mut self.state);

        let resolution = window.get_resolution();

        if !self.state.are_viewports_initialized {
            update_view_matrix(window.deref(), &mut self.state, &resolution.value);
            self.state.are_viewports_initialized = true;
        }

        //timer_start = std::chrono::high_resolution_clock::now();
        recompute_viewports(window, &mut self.state);
        compile_scenes(&self.vk_instance, &self.vk_device, window, &mut self.state);
        //timer_end = std::chrono::high_resolution_clock::now();
        //compile_time += (timer_end - timer_start);

        {
            //state.present_sem[state.cur_frame].wait();
        }

        let sc_image_index = get_next_image(&self.vk_device, &mut self.state, cur_frame);

        //timer_start = std::chrono::high_resolution_clock::now();
        record_scene_rebuild(
            &self.vk_instance,
            &self.vk_device,
            window,
            &mut self.state,
            cur_frame,
        );
        submit_scene_rebuild(&self.vk_device, &mut self.state, cur_frame);
        //timer_end = std::chrono::high_resolution_clock::now();
        //rebuild_time += (timer_end - timer_start);

        let canvas = window.get_canvas_mut().unwrap()
            .as_any_mut().downcast_mut::<RenderCanvas>().unwrap();

        {
            let mut viewports = canvas.get_viewports_2d().clone();
            viewports.sort_by_key(|vp| vp.get_z_index());
        }
        let viewports = canvas.get_viewports_2d();

        //timer_start = std::chrono::high_resolution_clock::now();
        for viewport in &viewports {
            draw_scene_to_framebuffer(
                &self.vk_instance,
                &self.vk_device,
                &mut self.state,
                viewport,
                canvas,
                resolution,
                cur_frame,
            );
        }

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
            &self.vk_device,
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
            MAX_FRAMES_IN_FLIGHT - 1,
            0,
            Ordering::Relaxed,
            Ordering::Relaxed,
        )
            .is_err() {
            self.state.cur_frame.fetch_add(1, Ordering::Relaxed);
        }

        //time_samples++;
    }

    pub(crate) fn notify_window_resize(&mut self, window: &Window, resolution: Vector2u) {
        update_view_matrix(window, &mut self.state, &resolution);

        let submit_ack_sem = Semaphore::default();
        self.state.submit_sender.as_ref().unwrap().send(
            SubmitMessage::NotifyDestroyedSwapchain(NotifyDestroyedSwapchainParams {
                swapchain: unsafe { self.state.swapchain.as_ref().unwrap().get_handle() },
                ack_sem: submit_ack_sem.clone(),
            })
        ).unwrap();
        submit_ack_sem.wait();

        let _submit_lock = self.state.submit_mutex.lock().unwrap();
        let _gfx_queue_lock = self.vk_device.queue_mutexes.graphics_family.lock().unwrap();
        let _present_queue_lock = if self.vk_device.queues.present_family != self.vk_device.queues.graphics_family {
            Some(self.vk_device.queue_mutexes.present_family.lock().unwrap())
        } else {
            None
        };

        unsafe {
            self.state.swapchain = Some(
                recreate_swapchain(
                    &self.vk_instance,
                    &self.vk_device,
                    self.state.swapchain.take().unwrap(),
                    resolution,
                    _submit_lock,
                    _gfx_queue_lock,
                    _present_queue_lock,
                ).unwrap()
            );
            self.state.submit_sender.as_ref().unwrap().send(
                SubmitMessage::NotifyCreatedSwapchain(NotifyCreatedSwapchainParams {
                    swapchain: self.state.swapchain.as_ref().unwrap().get_handle()
                })
            ).unwrap();
        }
    }
}

fn compute_proj_matrix(res_hor: u32, res_ver: u32) -> Matrix4x4 {
    // screen space is [0, 1] on both axes with the origin in the top-left
    let l = 0;
    let r = 1;
    let b = 1;
    let t = 0;

    let res_hor_f = res_hor as f32;
    let res_ver_f = res_ver as f32;

    //let scale_mode = EngineManager::instance().get_config().screen_scale_mode;
    //TODO
    let scale_mode = ScreenSpaceScaleMode::NormalizeMinDimension;
    let (scale_h, scale_v) = match scale_mode {
        ScreenSpaceScaleMode::NormalizeMinDimension => {
            if res_hor > res_ver {
                (res_hor_f / res_ver_f, 1.0)
            } else {
                (1.0, res_ver_f / res_hor_f)
            }
        }
        ScreenSpaceScaleMode::NormalizeMaxDimension => {
            if res_hor > res_ver {
                (1.0, res_ver_f / res_hor_f)
            } else {
                (res_hor_f / res_ver_f, 1.0)
            }
        }
        ScreenSpaceScaleMode::NormalizeVertical => {
            (res_hor_f / res_ver_f, 1.0)
        }
        ScreenSpaceScaleMode::NormalizeHorizontal => {
            (1.0, res_ver_f / res_hor_f)
        }
        ScreenSpaceScaleMode::None => {
            (1.0, 1.0)
        }
    };

    Matrix4x4::from_row_major([
        2.0 / ((r - l) as f32 * scale_h), 0.0, 0.0,
        -(r + l) as f32 / ((r - l) as f32 * scale_h),
        0.0, -2.0 / ((t - b) as f32 * scale_v), 0.0,
        -(t + b) as f32 / -((t - b) as f32 * scale_v),
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ])
}

fn recompute_2d_viewport_view_matrix(
    viewport: &Viewport,
    transform: &Transform2d,
    resolution: &Vector2u,
) -> Matrix4x4 {
    let center_x = (viewport.left + viewport.right) / 2.0;
    let center_y = (viewport.top + viewport.bottom) / 2.0;

    let cur_translation = transform.get_translation();

    let anchor_mat_1 = Matrix4x4::from_row_major([
        1.0, 0.0, 0.0, -center_x + cur_translation.x,
        0.0, 1.0, 0.0, -center_y + cur_translation.y,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ]);
    let anchor_mat_2 = Matrix4x4::from_row_major([
        1.0, 0.0, 0.0, center_x - cur_translation.x,
        0.0, 1.0, 0.0, center_y - cur_translation.y,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ]);
    let view_mat = transform.get_translation_matrix()
        * anchor_mat_2
        * transform.get_rotation_matrix()
        * transform.get_scale_matrix()
        * anchor_mat_1;

    compute_proj_matrix(resolution.x, resolution.y) * view_mat
}

fn get_associated_scenes_for_canvas(canvas: &RenderCanvas) -> HashSet<&str> {
    canvas.get_viewports_2d().iter()
        .map(|viewport| viewport.get_scene_id())
        .collect()
}

fn create_viewport_2d_state(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    command_pool: vk::CommandPool,
    viewport: &AttachedViewport2d
) -> ViewportState {
    let mut viewport_state = ViewportState::new(viewport.get_id());

    let sem_info = vk::SemaphoreCreateInfo::default();

    for frame_state in &mut viewport_state.per_frame {
        unsafe {
            frame_state.rebuild_semaphore = device.logical_device
                .create_semaphore(&sem_info, None)
                .expect("Failed to create semaphores for viewport");

            frame_state.draw_semaphore = device.logical_device
                .create_semaphore(&sem_info, None)
                .expect("Failed to create semaphores for viewport");

            let fence_info = vk::FenceCreateInfo::default()
                .flags(vk::FenceCreateFlags::empty());
            frame_state.composite_fence = device.logical_device
                .create_fence(&fence_info, None)
                .expect("Failed to create fences for viewport");
        }

        frame_state.command_buf = Some(alloc_command_buffers(
            device,
            command_pool,
            1,
        ).into_iter().next().unwrap());

        frame_state.scene_ubo = Some(VulkanBuffer::new(
            instance,
            device,
            SHADER_UBO_SCENE_LEN as vk::DeviceSize,
            vk::BufferUsageFlags::UNIFORM_BUFFER,
            MEM_CLASS_HOST_RW,
        ).unwrap());
    }

    viewport_state
}

fn create_scene_state(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    scene_id: impl Into<String>,
) -> Scene2dState {
    let mut scene_state = Scene2dState::new(scene_id.into());

    scene_state.ubo = Some(VulkanBuffer::new(
        instance,
        device,
        SHADER_UBO_SCENE_LEN as vk::DeviceSize,
        vk::BufferUsageFlags::UNIFORM_BUFFER,
        MEM_CLASS_HOST_RW,
    ).unwrap());

    scene_state
}

fn destroy_viewport(device: &VulkanDevice, desc_pool: vk::DescriptorPool, viewport_state: ViewportState) {
    for frame_state in viewport_state.per_frame {
        unsafe {
            device.logical_device.destroy_fence(frame_state.composite_fence, None);
            if let Some(fb) = frame_state.front_fb {
                if let Some(sampler) = fb.sampler {
                    device.logical_device.destroy_sampler(sampler, None);
                }
                for image in fb.images {
                    image.destroy(device);
                }
                destroy_framebuffer(device, fb.handle);
            }
            if let Some(fb) = frame_state.back_fb {
                if let Some(sampler) = fb.sampler {
                    device.logical_device.destroy_sampler(sampler, None);
                }
                for image in fb.images {
                    image.destroy(device);
                }
                destroy_framebuffer(device, fb.handle);
            }
        }

        if let Some(buf) = frame_state.viewport_ubo {
            buf.destroy(device);
        }
        if let Some(buf) = frame_state.scene_ubo {
            buf.destroy(device);
        }

        destroy_descriptor_sets(device, desc_pool, &frame_state.composite_desc_sets).unwrap();
        for (_, ds) in frame_state.material_desc_sets {
            destroy_descriptor_sets(device, desc_pool, &ds).unwrap();
        }

        destroy_command_buffer(device, frame_state.command_buf.unwrap());
    }
}

fn destroy_scene(device: &VulkanDevice, scene_state: Scene2dState) {
    for (_, bucket) in scene_state.render_buckets {
        bucket.destroy(device);
    }

    for (_, obj) in scene_state.processed_objs {
        obj.destroy(device);
    }

    if let Some(buf) = scene_state.ubo {
        buf.destroy(device);
    }
}

fn add_remove_state_objects(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    window: &Window,
    state: &mut RendererState,
) {
    let canvas = window.get_canvas().unwrap().as_any().downcast_ref::<RenderCanvas>().unwrap();

    for viewport in canvas.get_viewports_2d() {
        let vp_state = state.viewport_states_2d.entry(viewport.get_id())
            .or_insert_with(|| create_viewport_2d_state(
                instance,
                device,
                state.graphics_command_pool.unwrap(),
                viewport,
            ));

        vp_state.visited = true;

        let scene_id = viewport.get_scene_id();
        let scene_state = state.scene_states_2d.entry(scene_id.to_owned())
            .or_insert_with(|| create_scene_state(instance, device, scene_id));
        scene_state.visited = true;
    }

    // destroy scene states which were not visited this frame
    let stale_scene_states = state.scene_states_2d
        .extract_if(|_, scene_state| !scene_state.visited);
    for (_, scene_state) in stale_scene_states {
        destroy_scene(device, scene_state);
    }
    // clear visited flag for all remaining states
    for scene_state in state.scene_states_2d.values_mut() {
        scene_state.visited = false;
    }

    // destroy scene states which were not visited last frame
    let stale_viewport_states = state.viewport_states_2d
        .extract_if(|_, viewport_state| !viewport_state.visited);
    for (_, viewport_state) in stale_viewport_states {
        destroy_viewport(device, state.desc_pool.unwrap(), viewport_state);

        state.dirty_viewports = true;
    }
    // clear visited flag for all remaining states
    for viewport_state in state.viewport_states_2d.values_mut() {
        viewport_state.visited = false;
    }
}

fn update_view_matrix(window: &Window, state: &mut RendererState, resolution: &Vector2u) {
    let canvas = window.get_canvas().unwrap().as_any().downcast_ref::<RenderCanvas>().unwrap();

    for viewport in canvas.get_viewports_2d() {
        let viewport_state = state.viewport_states_2d.get_mut(&viewport.get_id()).unwrap();
        let scene = get_render_context_2d().get_scene(viewport.get_scene_id()).unwrap();
        let camera = scene.get_camera(viewport.get_camera_id()).unwrap();
        let camera_transform = camera.peek_transform();

        viewport_state.view_matrix = recompute_2d_viewport_view_matrix(
            canvas.get_viewport(viewport_state.viewport_id).unwrap().get_viewport(),
            &camera_transform.inverse(),
            resolution,
        );
        for frame_state in &mut viewport_state.per_frame {
            frame_state.view_matrix_dirty = true;
        }
    }
}

fn recompute_viewports(window: &Window, state: &mut RendererState) {
    let canvas = window.get_canvas().unwrap().as_any().downcast_ref::<RenderCanvas>().unwrap();

    for viewport in canvas.get_viewports_2d() {
        let viewport_state = state.viewport_states_2d.get_mut(&viewport.get_id()).unwrap();
        let mut scene = get_render_context_2d().get_scene_mut(viewport.get_scene_id()).unwrap();
        let camera = scene.get_camera_mut(viewport.get_camera_id()).unwrap();
        let camera_transform = camera.get_transform();

        if camera_transform.dirty {
            viewport_state.view_matrix = recompute_2d_viewport_view_matrix(
                viewport.get_viewport(),
                &camera_transform.value.inverse(),
                &window.peek_resolution(),
            );
        }
    }
}

fn compile_scenes(
    instance: &VulkanInstance,
    device: &VulkanDevice,
    window: &Window,
    state: &mut RendererState,
) {
    let canvas = window.get_canvas().unwrap().as_any().downcast_ref::<RenderCanvas>().unwrap();

    for scene_id in get_associated_scenes_for_canvas(canvas) {
        compile_scene_2d(instance, device, state, scene_id);
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
    instance: &VulkanInstance,
    device: &VulkanDevice,
    window: &Window,
    state: &mut RendererState,
    cur_frame: usize,
) {
    let canvas = window.get_canvas().unwrap().as_any().downcast_ref::<RenderCanvas>().unwrap();

    begin_oneshot_commands(device, state.copy_cmd_buf[cur_frame].as_ref().unwrap());

    for scene_id in get_associated_scenes_for_canvas(canvas) {
        check_scene_ubo_dirty(state, scene_id);

        let scene_state = state.scene_states_2d.get_mut(scene_id).unwrap();

        fill_buckets(
            instance,
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

            let prepared = prepare_texture(
                instance,
                device,
                copy_cmd_buf,
                texture_res,
            )
                .expect("Failed to prepare texture");

            state.prepared_textures.insert(texture_uid.clone(), prepared);
            state.material_textures.insert(
                material_res.get_prototype().uid.clone(),
                texture_uid.clone(),
            );
        }
    }

    end_command_buffer(device, state.copy_cmd_buf[cur_frame].as_ref().unwrap());
}

fn submit_scene_rebuild(device: &VulkanDevice, state: &mut RendererState, cur_frame: usize) {
    let rebuild_sems = state.viewport_states_2d.values()
        .map(|vp_state| vp_state.per_frame[cur_frame].rebuild_semaphore)
        .collect();
    queue_command_buffer_submit(
        state.submit_sender.as_ref().unwrap(),
        state.submit_mutex.as_ref(),
        state.copy_cmd_buf[cur_frame].as_ref().unwrap().clone(),
        state.swapchain.as_ref().unwrap(),
        device.queues.graphics_family,
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

fn get_next_image(device: &VulkanDevice, state: &mut RendererState, cur_frame: usize) -> u32 {
    let swapchain = &state.swapchain.as_ref().expect("Swapchain is not initialized");

    state.command_buffer_submitted_sem[cur_frame].wait();
    unsafe {
        device.logical_device.wait_for_fences(
            &[swapchain.in_flight_fence[cur_frame]],
            true,
            u64::MAX,
        )
            .expect("vkWaitForFences failed");
        device.logical_device.reset_fences(
            &[swapchain.in_flight_fence[cur_frame]],
        )
            .expect("vkResetFences failed");
    }

    let _submit_guard = state.submit_mutex.lock().unwrap();

    let (image_index, _) = unsafe {
        device.khr_swapchain().acquire_next_image(
            swapchain.get_handle(),
            u64::MAX,
            swapchain.image_avail_sem[cur_frame],
            vk::Fence::null(),
        ).expect("vkAcquireNextImageKHR failed")
    };

    image_index
}

fn composite_framebuffers(
    device: &VulkanDevice,
    state: &mut RendererState,
    viewports: &Vec<&AttachedViewport2d>,
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
        let new_cmd_buf = alloc_command_buffers(
            device,
            *state.graphics_command_pool.as_ref().unwrap(),
            1,
        ).into_iter().next().unwrap();
        state.composite_cmd_bufs.insert(sc_image_index, (new_cmd_buf, false));
        &state.composite_cmd_bufs.get(&sc_image_index).unwrap().0
    };

    let vk_cmd_buf = unsafe { cmd_buf.get_handle() };

    unsafe {
        device.logical_device.reset_command_buffer(
            vk_cmd_buf,
            vk::CommandBufferResetFlags::empty(),
        ).unwrap();
    }

    let cmd_begin_info = vk::CommandBufferBeginInfo::default()
        .flags(vk::CommandBufferUsageFlags::empty());
    unsafe {
        device.logical_device.begin_command_buffer(vk_cmd_buf, &cmd_begin_info).unwrap();
    }

    let swapchain = state.swapchain.as_ref().expect("Swapchain is not initialized");

    let fb_width = swapchain.extent.width;
    let fb_height = swapchain.extent.height;

    let clear_val = vk::ClearValue {
        color: vk::ClearColorValue { float32: [0.0, 0.0, 0.0, 1.0] },
    };
    let clear_vals = [clear_val];

    let rp_info = vk::RenderPassBeginInfo::default()
        .framebuffer(swapchain.framebuffers[sc_image_index as usize])
        .clear_values(&clear_vals)
        .render_pass(swapchain.composite_render_pass)
        .render_area(vk::Rect2D {
            extent: vk::Extent2D { width: fb_width, height: fb_height },
            offset: vk::Offset2D { x: 0, y: 0 },
        });
    unsafe {
        device.logical_device
            .cmd_begin_render_pass(vk_cmd_buf, &rp_info, vk::SubpassContents::INLINE);
    }

    unsafe {
        device.logical_device.cmd_bind_pipeline(
            vk_cmd_buf,
            vk::PipelineBindPoint::GRAPHICS,
            state.composite_pipeline.as_ref().unwrap().get_handle(),
        );
    }

    unsafe {
        device.logical_device.cmd_bind_vertex_buffers(
            vk_cmd_buf,
            0,
            &[state.composite_vbo.as_ref().unwrap().get_handle()],
            &[0],
        );
    }

    for viewport in viewports {
        let viewport_state = state.viewport_states_2d.get_mut(&viewport.get_id()).unwrap();

        draw_framebuffer_to_swapchain(
            device,
            &mut viewport_state.per_frame[cur_frame],
            viewport.get_viewport(),
            state.swapchain.as_ref().unwrap(),
            state.composite_pipeline.as_ref().unwrap(),
            state.composite_cmd_bufs.get(&sc_image_index).unwrap().0.clone(),
        );
    }

    unsafe {
        device.logical_device.cmd_end_render_pass(vk_cmd_buf);
    }

    unsafe {
        device.logical_device.end_command_buffer(vk_cmd_buf).unwrap();
    }
}

fn submit_composite(
    device: &VulkanDevice,
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
        wait_sems.push(viewport_state.per_frame[cur_frame].draw_semaphore);
        wait_stages.push(vk::PipelineStageFlags::ALL_COMMANDS);
    }

    queue_command_buffer_submit(
        state.submit_sender.as_ref().unwrap(),
        state.submit_mutex.as_ref(),
        state.composite_cmd_bufs.get(&sc_image_index).unwrap().0.clone(),
        state.swapchain.as_ref().unwrap(),
        device.queues.graphics_family,
        wait_sems,
        wait_stages,
        vec![swapchain.render_done_sem[cur_frame]],
        Some(swapchain.in_flight_fence[cur_frame]),
        Some(state.command_buffer_submitted_sem[cur_frame].clone()),
    )
        .unwrap();
}

fn present_image(state: &mut RendererState, image_index: u32, cur_frame: usize) {
    let swapchain = state.swapchain.as_ref().unwrap();
    state.submit_sender.as_ref().unwrap().send(SubmitMessage::PresentImage(PresentImageParams {
        swapchain: unsafe { swapchain.get_handle() },
        wait_sems: vec![
            swapchain.render_done_sem[cur_frame],
        ],
        present_image_index: image_index,
        present_sem: state.present_submitted_sem[cur_frame].clone(),
    })).unwrap();
}

fn start_submit_queues_thread(
    device: ash::Device,
    ext_khr_swapchain: khr::swapchain::Device,
    initial_swapchain: &VulkanSwapchain,
    submit_mutex: Arc<Mutex<()>>,
    graphics_queue: vk::Queue,
    graphics_queue_mutex: Arc<Mutex<()>>,
    present_queue_mutex: Option<Arc<Mutex<()>>>,
    receiver: mpsc::Receiver<SubmitMessage>,
) -> JoinHandle<()> {
    let initial_swapchain_handle = unsafe { initial_swapchain.get_handle() };
    thread::spawn(move || {
        // We track which swapchain is currently active based on notifications
        // sent from the main thread. This allows ignoring submit/present
        // requests associated with a stale swapchain.
        let mut cur_swapchain = Some(initial_swapchain_handle);

        'outer: loop {

            while let Ok(message) = receiver.recv() {
                let _submit_lock = submit_mutex.lock().unwrap();
                let _gfx_queue_lock = graphics_queue_mutex.lock().unwrap();
                let _present_queue_lock = present_queue_mutex.as_ref()
                    .map(|present_queue_mutex| present_queue_mutex.lock().unwrap());

                match message {
                    SubmitMessage::PresentImage(present_params) => {
                        if cur_swapchain.is_none_or(|sc| sc != present_params.swapchain) {
                            continue;
                        }

                        let swapchains = [present_params.swapchain];
                        let image_indices = [present_params.present_image_index];

                        let present_info = vk::PresentInfoKHR::default()
                            .wait_semaphores(&present_params.wait_sems)
                            .swapchains(&swapchains)
                            .image_indices(&image_indices);

                        unsafe {
                            ext_khr_swapchain
                                .queue_present(graphics_queue, &present_info)
                                .unwrap();
                        }

                        present_params.present_sem.notify();
                    }
                    SubmitMessage::SubmitCommandBuffer(buf_params) => {
                        if cur_swapchain.is_none_or(|sc| sc != buf_params.swapchain) {
                            continue;
                        }

                        submit_command_buffer(
                            &device,
                            &buf_params.buffer,
                            buf_params.queue,
                            buf_params.fence.unwrap_or(vk::Fence::null()),
                            buf_params.wait_sems,
                            buf_params.wait_stages,
                            buf_params.signal_sems,
                        );

                        if let Some(submit_sem) = buf_params.in_flight_sem {
                            submit_sem.notify();
                        }
                    }
                    SubmitMessage::NotifyCreatedSwapchain(sc_params) => {
                        assert!(cur_swapchain.is_none());
                        cur_swapchain = Some(sc_params.swapchain);
                    }
                    SubmitMessage::NotifyDestroyedSwapchain(sc_params) => {
                        assert!(cur_swapchain.is_some_and(|sc| sc == sc_params.swapchain));
                        cur_swapchain = None;
                        sc_params.ack_sem.notify();
                    }
                    SubmitMessage::NotifyHalting(halting_params) => {
                        halting_params.ack_sem.notify();
                        break 'outer;
                    }
                }
            }
        }
    })
}
