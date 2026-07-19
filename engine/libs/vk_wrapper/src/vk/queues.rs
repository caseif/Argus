use std::marker::PhantomData;
use std::sync::{mpsc, Arc, Mutex};
use std::thread;
use std::thread::JoinHandle;
use ash::vk::Handle;
use crate::vk;
use crate::vk::Wrapper;

#[derive(Clone, Copy, Debug)]
pub struct QueueFamilyIndices {
    pub graphics_family: u32,
    pub transfer_family: u32,
    pub present_family: u32,
}

pub struct QueueFamilies<'ctx> {
    pub graphics_family: Queue<'ctx>,
    #[allow(dead_code)]
    pub transfer_family: Queue<'ctx>,
    pub present_family: Queue<'ctx>,
}

#[derive(Clone, Debug, Default)]
pub struct QueueMutexes {
    pub graphics_family: Arc<Mutex<()>>,
    #[allow(dead_code)]
    pub transfer_family: Arc<Mutex<()>>,
    pub present_family: Arc<Mutex<()>>,
}

pub struct Queue<'ctx> {
    device: ash::Device,
    underlying: ash::vk::Queue,
    _phantom: PhantomData<PhantomData<&'ctx vk::Device<'ctx>>>
}

impl<'ctx> PartialEq for Queue<'ctx> {
    fn eq(&self, other: &Self) -> bool {
        self.underlying == other.underlying
    }
}

impl<'ctx> Wrapper for Queue<'ctx> {
    type Underlying = ash::vk::Queue;

    unsafe fn get_underlying(&self) -> ash::vk::Queue {
        self.underlying
    }
}

impl<'ctx> Queue<'ctx> {
    pub(crate) unsafe fn from_underlying(device: &ash::Device, underlying: ash::vk::Queue) -> Self {
        Self {
            device: device.clone(),
            underlying,
            _phantom: PhantomData::default(),
        }
    }

    pub unsafe fn from_handle(device: &ash::Device, handle: u64) -> Self {
        unsafe { Self::from_underlying(device, ash::vk::Queue::from_raw(handle)) }
    }

    pub unsafe fn get_handle(&self) -> u64 {
        self.underlying.as_raw()
    }

    pub fn wait_idle(&self) -> Result<(), String> {
        unsafe {
            self.device.queue_wait_idle(self.underlying)
                .map_err(|err| err.to_string())
        }
    }
}

pub fn start_submit_queues_thread(
    device: &vk::Device,
    initial_swapchain: &vk::Swapchain,
    submit_mutex: Arc<Mutex<()>>,
    graphics_queue: &Queue,
    graphics_queue_mutex: Arc<Mutex<()>>,
    present_queue_mutex: Option<Arc<Mutex<()>>>,
    receiver: mpsc::Receiver<vk::SubmitMessage>,
) -> JoinHandle<()> {
    let device_handle = unsafe { device.get_underlying() }.clone();
    let ext_khr_swapchain_handle = device.ext_khr_swapchain.clone();
    let initial_swapchain_handle = unsafe { initial_swapchain.get_underlying() };
    let graphics_queue_handle = unsafe { graphics_queue.get_handle() };
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
                    vk::SubmitMessage::PresentImage(present_params) => {
                        if cur_swapchain.is_none_or(|sc| sc != present_params.swapchain) {
                            continue;
                        }

                        let swapchains = [present_params.swapchain];
                        let image_indices = [present_params.present_image_index];

                        let present_info = ash::vk::PresentInfoKHR::default()
                            .wait_semaphores(&present_params.wait_sems)
                            .swapchains(&swapchains)
                            .image_indices(&image_indices);

                        let graphics_queue = unsafe {
                            Queue::from_handle(
                                &device_handle,
                                graphics_queue_handle,
                            )
                        };
                        unsafe {
                            ext_khr_swapchain_handle
                                .queue_present(graphics_queue.get_underlying(), &present_info)
                                .unwrap();
                        }

                        present_params.present_sem.notify();
                    }
                    vk::SubmitMessage::SubmitCommandBuffer(buf_params) => {
                        if cur_swapchain.is_none_or(|sc| sc != buf_params.swapchain) {
                            continue;
                        }

                        unsafe {
                            vk::submit_command_buffer(
                                &buf_params.device,
                                buf_params.buffer,
                                buf_params.queue,
                                buf_params.fence,
                                buf_params.wait_sems,
                                buf_params.wait_stages,
                                buf_params.signal_sems,
                            );
                        }

                        if let Some(submit_sem) = buf_params.in_flight_sem {
                            submit_sem.notify();
                        }
                    }
                    vk::SubmitMessage::NotifyCreatedSwapchain(sc_params) => {
                        assert!(cur_swapchain.is_none());
                        cur_swapchain = Some(sc_params.swapchain);
                    }
                    vk::SubmitMessage::NotifyDestroyedSwapchain(sc_params) => {
                        assert!(cur_swapchain.is_some_and(|sc| sc == sc_params.swapchain));
                        cur_swapchain = None;
                        sc_params.ack_sem.notify();
                    }
                    vk::SubmitMessage::NotifyHalting(halting_params) => {
                        halting_params.ack_sem.notify();
                        break 'outer;
                    }
                }
            }
        }
    })
}
