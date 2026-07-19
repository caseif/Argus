use std::sync::{mpsc, Mutex};
use std::sync::mpsc::SendError;
use argus_util::semaphore::Semaphore;
use crate::vk;

pub use ash::vk::CommandBufferBeginInfo;
pub use ash::vk::CommandBufferResetFlags;
pub use ash::vk::CommandBufferUsageFlags;
use crate::vk::{Wrapper, WrapperIterator};

pub struct CommandPool<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::CommandPool,
}

impl<'ctx> Wrapper for CommandPool<'ctx> {
    type Underlying = ash::vk::CommandPool;

    unsafe fn get_underlying(&self) -> ash::vk::CommandPool {
        self.underlying
    }
}

impl<'ctx> CommandPool<'ctx> {
    pub fn create(device: &'ctx vk::Device, queue_index: u32) -> Self {
        let pool_info = ash::vk::CommandPoolCreateInfo::default()
            .flags(ash::vk::CommandPoolCreateFlags::RESET_COMMAND_BUFFER)
            .queue_family_index(queue_index);

        let pool = unsafe {
            device.get_underlying().create_command_pool(&pool_info, None)
                .expect("Failed to create command pool")
        };
        Self {
            device,
            underlying: pool
        }
    }

    pub fn destroy(self) {
        unsafe { self.device.get_underlying().destroy_command_pool(self.underlying, None) };
    }

    pub fn alloc_buffers(
        &self,
        count: u32,
    ) -> Vec<CommandBuffer<'ctx>> {
        let cb_alloc_info = ash::vk::CommandBufferAllocateInfo::default()
            .command_pool(self.underlying)
            .level(ash::vk::CommandBufferLevel::PRIMARY)
            .command_buffer_count(count);

        let handles = unsafe {
            self.device.get_underlying().allocate_command_buffers(&cb_alloc_info)
                .expect("Failed to allocate command buffers")
        };

        handles.into_iter()
            .map(|handle| CommandBuffer {
                device: self.device,
                vk_pool: self.underlying,
                underlying: handle,
            })
            .collect()
    }
}

pub struct CommandBuffer<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::CommandBuffer,
    vk_pool: ash::vk::CommandPool, // for validation only
}

impl<'ctx> Wrapper for CommandBuffer<'ctx> {
    type Underlying = ash::vk::CommandBuffer;

    unsafe fn get_underlying(&self) -> ash::vk::CommandBuffer {
        self.underlying
    }
}

impl<'ctx> CommandBuffer<'ctx> {
    pub fn destroy(self, pool: &CommandPool) {
        assert_eq!(unsafe { pool.get_underlying() }, self.vk_pool);
        unsafe {
            self.device.get_underlying()
                .free_command_buffers(pool.get_underlying(), &[self.underlying]);
        }
    }

    pub fn begin_oneshot_commands(&self) {
        unsafe {
            self.device.get_underlying()
                .reset_command_buffer(self.underlying, vk::CommandBufferResetFlags::empty())
                .expect("Failed to reset command buffer");
        }

        let begin_info = vk::CommandBufferBeginInfo::default()
            .flags(vk::CommandBufferUsageFlags::ONE_TIME_SUBMIT);
        unsafe {
            self.device.get_underlying()
                .begin_command_buffer(self.underlying, &begin_info)
                .expect("Failed to begin command buffer"); //TODO
        };
    }

    pub fn begin_commands(&self, begin_info: &CommandBufferBeginInfo) -> Result<(), String> {
        unsafe {
            self.device.get_underlying()
                .begin_command_buffer(self.underlying, &begin_info)
                .map_err(|err| err.to_string())
        }
    }

    pub fn end_commands(&self) {
        unsafe {
            self.device.get_underlying()
                .end_command_buffer(self.underlying)
                .expect("Failed to end command buffer"); //TODO
        }
    }

    pub fn reset(&self, flags: vk::CommandBufferResetFlags) {
        unsafe {
            self.device.get_underlying()
                .reset_command_buffer(self.underlying, flags).unwrap();
        }
    }
    
    pub fn cmd_set_viewport(&self, first_viewport: u32, viewports: &[vk::Viewport]) {
        unsafe {
            self.device.get_underlying()
                .cmd_set_viewport(self.underlying, first_viewport, viewports);
        }
    }

    pub fn cmd_set_scissor(&self, first_scissor: u32, scissors: &[vk::Rect2D]) {
        unsafe {
            self.device.get_underlying()
                .cmd_set_scissor(self.underlying, first_scissor, scissors);
        }
    }

    pub fn cmd_begin_render_pass(
        &self,
        begin_info: &vk::RenderPassBeginInfo,
        subpass: vk::SubpassContents,
    ) {
        unsafe {
            self.device.get_underlying()
                .cmd_begin_render_pass(self.underlying, &begin_info.create_underlying(), subpass);
        }
    }

    pub fn cmd_end_render_pass(&self) {
        unsafe {
            self.device.get_underlying()
                .cmd_end_render_pass(self.underlying);
        }
    }

    pub fn cmd_bind_pipeline(
        &self,
        bind_point: vk::PipelineBindPoint,
        pipeline: &vk::Pipeline<'ctx>,
    ) {
        unsafe {
            self.device.get_underlying()
                .cmd_bind_pipeline(self.underlying, bind_point, pipeline.get_underlying());
        }
    }

    pub fn cmd_bind_vertex_buffers(
        &self,
        first_binding: u32,
        buffers: &[&vk::Buffer<'ctx>],
        offsets: &[vk::DeviceSize],
    ) {
        unsafe {
            self.device.get_underlying()
                .cmd_bind_vertex_buffers(
                    self.underlying,
                    first_binding,
                    &buffers.iter().map(|buf| buf.get_underlying()).collect::<Vec<_>>(),
                    offsets,
                )
        }
    }
    
    pub fn cmd_bind_descriptor_sets(
        &self,
        pipeline_bind_point: vk::PipelineBindPoint,
        layout: &vk::PipelineLayout,
        first_set: u32,
        descriptor_sets: &[&vk::DescriptorSet],
        dynamic_offsets: &[u32],
    ) {
        unsafe {
            self.device.get_underlying()
                .cmd_bind_descriptor_sets(
                    self.underlying,
                    pipeline_bind_point,
                    layout.get_underlying(),
                    first_set,
                    &descriptor_sets.iter().map(|ds| ds.get_underlying()).collect::<Vec<_>>(),
                    dynamic_offsets,
            );
        }
    }

    pub fn cmd_draw(
        &self,
        vertex_count: u32,
        instance_count: u32,
        first_vertex: u32,
        first_instance: u32,
    ) {
        unsafe {
            self.device.get_underlying()
                .cmd_draw(
                    self.underlying,
                    vertex_count,
                    instance_count,
                    first_vertex,
                    first_instance,
                );
        }
    }

    #[allow(dead_code)]
    pub fn submit(
        &self,
        queue: ash::vk::Queue,
        fence: Option<ash::vk::Fence>,
        wait_semaphores: Vec<ash::vk::Semaphore>,
        wait_stages: Vec<ash::vk::PipelineStageFlags>,
        signal_semaphores: Vec<ash::vk::Semaphore>,
    ) {
        unsafe {
            submit_command_buffer(
                self.device.get_underlying(),
                self.underlying,
                queue,
                fence,
                wait_semaphores,
                wait_stages,
                signal_semaphores,
            )
        }
    }

    pub fn queue_submit(
        &self,
        sender: &mpsc::Sender<SubmitMessage>,
        submit_mutex: &Mutex<()>,
        swapchain: &vk::Swapchain,
        queue: &vk::Queue<'ctx>,
        wait_semaphores: Vec<vk::Semaphore<'ctx>>,
        wait_stages: Vec<vk::PipelineStageFlags>,
        signal_semaphores: Vec<vk::Semaphore<'ctx>>,
        fence: Option<vk::Fence<'ctx>>,
        in_flight_sem: Option<Semaphore>,
    ) -> Result<(), SendError<SubmitMessage>> {
        let _lock = submit_mutex.lock().unwrap();
        let params = {
            SubmitCommandBufferParams {
                device: unsafe { self.device.get_underlying().clone() },
                buffer: unsafe { self.get_underlying() },
                swapchain: unsafe { swapchain.get_underlying() },
                queue: unsafe { queue.get_underlying() },
                wait_sems: unsafe { wait_semaphores.get_underlying() },
                wait_stages,
                signal_sems: unsafe { signal_semaphores.get_underlying() },
                fence: unsafe { fence.map(|f| f.get_underlying()) },
                in_flight_sem,
            }
        };
        sender.send(SubmitMessage::SubmitCommandBuffer(params))?;
        Ok(())
    }
}

pub enum SubmitMessage {
    SubmitCommandBuffer(SubmitCommandBufferParams),
    PresentImage(PresentImageParams),
    NotifyCreatedSwapchain(NotifyCreatedSwapchainParams),
    NotifyDestroyedSwapchain(NotifyDestroyedSwapchainParams),
    NotifyHalting(NotifyHaltingParams),
}

pub struct SubmitCommandBufferParams {
    pub device: ash::Device,
    pub buffer: ash::vk::CommandBuffer,
    pub swapchain: ash::vk::SwapchainKHR,
    pub queue: ash::vk::Queue,
    pub wait_sems: Vec<ash::vk::Semaphore>,
    pub wait_stages: Vec<ash::vk::PipelineStageFlags>,
    pub signal_sems: Vec<ash::vk::Semaphore>,
    pub fence: Option<ash::vk::Fence>,
    pub in_flight_sem: Option<Semaphore>,
}

pub struct PresentImageParams {
    pub swapchain: ash::vk::SwapchainKHR,
    pub wait_sems: Vec<ash::vk::Semaphore>,
    pub present_image_index: u32,
    pub present_sem: Semaphore,
}

impl PresentImageParams {
    pub fn new(
        swapchain: &vk::Swapchain,
        wait_sems: Vec<vk::Semaphore>,
        present_image_index: u32,
        present_sem: Semaphore,
    ) -> Self {
        Self {
            swapchain: unsafe { swapchain.get_underlying() },
            wait_sems: wait_sems.into_iter().map(|w| unsafe { w.get_underlying() }).collect(),
            present_image_index,
            present_sem,
        }
    }
}

pub struct NotifyCreatedSwapchainParams {
    pub swapchain: ash::vk::SwapchainKHR,
}

impl NotifyCreatedSwapchainParams {
    pub fn new(swapchain: &vk::Swapchain) -> Self {
        Self {
            swapchain: unsafe { swapchain.get_underlying() },
        }
    }
}

pub struct NotifyDestroyedSwapchainParams {
    pub swapchain: ash::vk::SwapchainKHR,
    pub ack_sem: Semaphore,
}

impl NotifyDestroyedSwapchainParams {
    pub fn new(swapchain: &vk::Swapchain, ack_sem: Semaphore) -> Self {
        Self {
            swapchain: unsafe { swapchain.get_underlying() },
            ack_sem,
        }
    }
}

pub struct NotifyHaltingParams {
    pub ack_sem: Semaphore,
}

pub unsafe fn submit_command_buffer(
    device: &ash::Device,
    buffer: ash::vk::CommandBuffer,
    queue: ash::vk::Queue,
    fence: Option<ash::vk::Fence>,
    wait_semaphores: Vec<ash::vk::Semaphore>,
    wait_stages: Vec<ash::vk::PipelineStageFlags>,
    signal_semaphores: Vec<ash::vk::Semaphore>,
) {
    assert_eq!(wait_semaphores.len(), wait_stages.len());

    let buffers = [buffer];

    let submit_info = ash::vk::SubmitInfo::default()
        .command_buffers(&buffers)
        .wait_semaphores(&wait_semaphores)
        .wait_dst_stage_mask(&wait_stages)
        .signal_semaphores(&signal_semaphores);
    let submit_infos = [submit_info];
    unsafe {
        device.queue_submit(
            queue,
            &submit_infos,
            fence.unwrap_or(ash::vk::Fence::null())
        )
            .expect("Failed to submit command queues");
    }
}
