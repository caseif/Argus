use std::collections::HashMap;
use std::sync::{mpsc, Arc, Mutex};
use std::sync::atomic::AtomicUsize;
use std::thread::JoinHandle;
use argus_resman::ResourceIdentifier;
use argus_util::math::Vector2u;
use argus_util::semaphore::Semaphore;
use vk_wrapper::vk;
use crate::state::{Scene2dState, ViewportState};

pub(crate) struct RendererState<'ctx> {
    pub(crate) device: &'ctx vk::Device<'ctx>,

    pub(crate) viewport_size: Vector2u,

    pub(crate) swapchain: Option<vk::Swapchain<'ctx>>,

    pub(crate) composite_pipeline: Option<vk::Pipeline<'ctx>>,
    pub(crate) composite_vbo: Option<vk::Buffer<'ctx>>,

    pub(crate) graphics_command_pool: Option<vk::CommandPool<'ctx>>,
    pub(crate) desc_pool: Option<vk::DescriptorPool<'ctx>>,

    pub(crate) fb_render_pass: Option<vk::RenderPass<'ctx>>,

    pub(crate) cur_frame: Arc<AtomicUsize>,

    pub(crate) copy_cmd_buf: [Option<vk::CommandBuffer<'ctx>>; vk::MAX_FRAMES_IN_FLIGHT],
    pub(crate) composite_cmd_bufs: HashMap<u32, (vk::CommandBuffer<'ctx>, bool)>,

    pub(crate) global_ubo: Option<vk::Buffer<'ctx>>,

    pub(crate) scene_states_2d: HashMap<String, Scene2dState<'ctx>>,
    pub(crate) viewport_states_2d: HashMap<u32, ViewportState<'ctx>>,
    pub(crate) are_viewports_initialized: bool,

    pub(crate) dirty_viewports: bool,

    pub(crate) material_pipelines: HashMap<ResourceIdentifier, vk::Pipeline<'ctx>>,
    pub(crate) prepared_textures: HashMap<ResourceIdentifier, vk::PreparedTexture<'ctx>>,
    pub(crate) material_textures: HashMap<ResourceIdentifier, ResourceIdentifier>,

    pub(crate) submit_thread: Option<JoinHandle<()>>,
    pub(crate) submit_sender: Option<mpsc::Sender<vk::SubmitMessage>>,
    pub(crate) submit_mutex: Arc<Mutex<()>>,
    pub(crate) submit_halt: bool,
    pub(crate) submit_halt_acked: Semaphore,

    pub(crate) present_submitted_sem: [Semaphore; vk::MAX_FRAMES_IN_FLIGHT],
    pub(crate) command_buffer_submitted_sem: [Semaphore; vk::MAX_FRAMES_IN_FLIGHT],
}

impl<'ctx> RendererState<'ctx> {
    pub fn new(device: &'ctx vk::Device<'ctx>) -> Self {
        Self {
            device,
            viewport_size: Default::default(),
            swapchain: Default::default(),
            composite_pipeline: Default::default(),
            composite_vbo: Default::default(),
            graphics_command_pool: Default::default(),
            desc_pool: Default::default(),
            fb_render_pass: Default::default(),
            cur_frame: Default::default(),
            copy_cmd_buf: Default::default(),
            composite_cmd_bufs: Default::default(),
            global_ubo: Default::default(),
            scene_states_2d: Default::default(),
            viewport_states_2d: Default::default(),
            are_viewports_initialized: Default::default(),
            dirty_viewports: Default::default(),
            material_pipelines: Default::default(),
            prepared_textures: Default::default(),
            material_textures: Default::default(),
            submit_thread: Default::default(),
            submit_sender: Default::default(),
            submit_mutex: Default::default(),
            submit_halt: Default::default(),
            submit_halt_acked: Default::default(),
            present_submitted_sem: Default::default(),
            command_buffer_submitted_sem: Default::default(),
        }
    }
}
