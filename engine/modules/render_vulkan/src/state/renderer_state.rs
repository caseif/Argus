use std::collections::HashMap;
use std::sync::{mpsc, Arc, Mutex};
use std::sync::atomic::AtomicUsize;
use std::thread::JoinHandle;
use ash::vk;
use argus_resman::{Resource, ResourceIdentifier};
use argus_util::math::Vector2u;
use argus_util::semaphore::Semaphore;
use crate::setup::swapchain::VulkanSwapchain;
use crate::state::{Scene2dState, ViewportState};
use crate::util::{CommandBufferInfo, PipelineInfo, PreparedTexture, VulkanBuffer};
use crate::util::defines::MAX_FRAMES_IN_FLIGHT;

pub(crate) enum SubmitMessage {
    SubmitCommandBuffer(SubmitCommandBufferParams),
    PresentImage(PresentImageParams),
}

pub(crate) struct SubmitCommandBufferParams {
    pub(crate) buffer: CommandBufferInfo,
    pub(crate) queue: vk::Queue,
    pub(crate) wait_sems: Vec<vk::Semaphore>,
    pub(crate) wait_stages: Vec<vk::PipelineStageFlags>,
    pub(crate) signal_sems: Vec<vk::Semaphore>,
    pub(crate) fence: Option<vk::Fence>,
    pub(crate) in_flight_sem: Option<Semaphore>,
}

pub(crate) struct PresentImageParams {
    pub(crate) swapchain: vk::SwapchainKHR,
    pub(crate) wait_sems: Vec<vk::Semaphore>,
    pub(crate) present_image_index: u32,
    pub(crate) present_sem: Semaphore,
}

#[derive(Default)]
pub(crate) struct RendererState {
    pub(crate) viewport_size: Vector2u,

    pub(crate) surface: Option<vk::SurfaceKHR>,
    pub(crate) swapchain: Option<VulkanSwapchain>,

    pub(crate) composite_pipeline: Option<PipelineInfo>,
    pub(crate) composite_vbo: Option<VulkanBuffer>,

    pub(crate) graphics_command_pool: Option<vk::CommandPool>,
    pub(crate) desc_pool: Option<vk::DescriptorPool>,

    pub(crate) fb_render_pass: Option<vk::RenderPass>,

    pub(crate) cur_frame: Arc<AtomicUsize>,

    pub(crate) copy_cmd_buf: [Option<CommandBufferInfo>; MAX_FRAMES_IN_FLIGHT],
    pub(crate) composite_cmd_bufs: HashMap<u32, (CommandBufferInfo, bool)>,

    pub(crate) global_ubo: Option<VulkanBuffer>,

    pub(crate) scene_states_2d: HashMap<String, Scene2dState>,
    pub(crate) viewport_states_2d: HashMap<u32, ViewportState>,
    pub(crate) are_viewports_initialized: bool,

    pub(crate) dirty_viewports: bool,

    pub(crate) material_resources: HashMap<String, Resource>,
    pub(crate) material_pipelines: HashMap<ResourceIdentifier, PipelineInfo>,
    pub(crate) prepared_textures: HashMap<ResourceIdentifier, PreparedTexture>,
    pub(crate) material_textures: HashMap<ResourceIdentifier, ResourceIdentifier>,
    pub(crate) texture_bufs_to_free: Vec<VulkanBuffer>,

    pub(crate) composite_semaphore: vk::Semaphore,

    pub(crate) submit_thread: Option<JoinHandle<()>>,
    pub(crate) submit_sender: Option<mpsc::Sender<SubmitMessage>>,
    pub(crate) submit_mutex: Arc<Mutex<()>>,
    pub(crate) submit_halt: bool,
    pub(crate) submit_halt_acked: Semaphore,

    pub(crate) present_submitted_sem: [Semaphore; MAX_FRAMES_IN_FLIGHT],
    pub(crate) command_buffer_submitted_sem: [Semaphore; MAX_FRAMES_IN_FLIGHT],
}
