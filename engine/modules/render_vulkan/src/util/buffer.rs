use std::cell::RefCell;
use std::slice;
use std::sync::atomic::{AtomicU64, Ordering};
use ash::prelude::VkResult;
use ash::vk;
use ash::vk::Handle;
use crate::setup::device::VulkanDevice;
use crate::setup::instance::VulkanInstance;
use crate::util::{find_memory_type, CommandBufferInfo};

static ALLOCATION_COUNT: AtomicU64 = AtomicU64::new(0);

pub(crate) struct VulkanBuffer {
    handle: vk::Buffer,
    mem: vk::DeviceMemory,
    size: vk::DeviceSize,
    usage_flags: vk::BufferUsageFlags,
    is_map_in_use: RefCell<bool>,
    persistent_map: Option<*mut u8>,
    is_destroyed: bool,
}

pub(crate) struct MappedBuffer<'a> {
    buffer: &'a VulkanBuffer,
    pub(crate) map: &'a mut [u8],
}

impl Drop for MappedBuffer<'_> {
    fn drop(&mut self) {
        *self.buffer.is_map_in_use.borrow_mut() = false;
        // a persistent map should only be unmapped when the buffer is destroyed
        if self.buffer.persistent_map.is_some() {
            return;
        }
    }
}

impl MappedBuffer<'_> {
    #[allow(unused)]
    pub(crate) fn as_slice<T>(&self) -> &[T] {
        assert_eq!(
            self.map.len() % size_of::<T>(),
            0,
            "Slice element type size is not divisor of buffer size",
        );
        unsafe {
            slice::from_raw_parts(
                self.map.as_ptr().cast(),
                self.map.len() / size_of::<T>(),
            )
        }
    }

    pub(crate) fn as_slice_mut<T>(&mut self) -> &mut [T] {
        assert_eq!(
            self.map.len() % size_of::<T>(),
            0,
            "Slice element type size is not divisor of buffer size",
        );
        unsafe {
            slice::from_raw_parts_mut(
                self.map.as_mut_ptr().cast(),
                self.map.len() / size_of::<T>(),
            )
        }
    }
    
    pub(crate) fn offset(&self, offset: impl Into<vk::DeviceSize>) -> &[u8] {
        &self.map[offset.into() as usize..]
    }

    pub(crate) fn offset_mut(&mut self, offset: impl Into<vk::DeviceSize>) -> &mut [u8] {
        &mut self.map[offset.into() as usize..]
    }
}

impl VulkanBuffer {
    pub(crate) fn new(
        instance: &VulkanInstance,
        device: &VulkanDevice,
        size: vk::DeviceSize,
        usage: vk::BufferUsageFlags,
        props: vk::MemoryPropertyFlags,
    ) -> Result<Self, String> {
        assert!(size > 0);

        let buffer_info = vk::BufferCreateInfo::default()
            .size(size)
            .usage(usage)
            .sharing_mode(vk::SharingMode::EXCLUSIVE);

        // SAFETY:
        // UID-vkCreateBuffer-device-parameter:
        //   The device object is guaranteed to contain a valid handle.
        // VUID-vkCreateBuffer-device-09664:
        //   Device creation can only succeed if both video and transfer
        //   queue families are supported.
        // VUID-vkCreateBuffer-flags-00911,
        // VUID-vkCreateBuffer-flags-09383,
        // VUID-vkCreateBuffer-flags-09384:
        //   We do not set any buffer creation flags.
        // VUID-vkCreateBuffer-pNext-06387:
        //   We do not use FUCHSIA extensions.
        let buffer = unsafe {
            device.logical_device.create_buffer(&buffer_info, None)
                .map_err(|err| err.to_string())?
        };

        // SAFETY: We just created the buffer handle so we know it's valid and
        // was created on the same device.
        let mem_reqs = unsafe { device.logical_device.get_buffer_memory_requirements(buffer) };

        let Some(mem_type_index) =
            find_memory_type(instance, device, mem_reqs.memory_type_bits, mem_reqs.size, props)
        else {
            return Err("Failed to find suitable memory type for buffer".to_owned());
        };
        let alloc_info = vk::MemoryAllocateInfo::default()
            .allocation_size(mem_reqs.size)
            .memory_type_index(mem_type_index);

        if ALLOCATION_COUNT.fetch_add(1, Ordering::Relaxed) >
            device.limits.max_memory_allocation_count as u64 {
            ALLOCATION_COUNT.fetch_sub(1, Ordering::Relaxed);
            return Err("Reached max memory allocation count".to_owned());
        }

        // SAFETY:
        // VUID-vkAllocateMemory-pAllocateInfo-01713:
        //   find_memory_type guarantees that the corresponding heap size of the
        //   returned type is at least as large as the allocation size.
        // VUID-vkAllocateMemory-pAllocateInfo-01714:
        //   find_memory_type will not return an out-of-bounds index.
        // VUID-vkAllocateMemory-deviceCoherentMemory-02790:
        //   find_memory_type will not return a memory type which supports
        //   DEVICE_COHERENT_BIT_AMD.
        // VUID-vkAllocateMemory-maxMemoryAllocationCount-04101:
        //   We check total allocations above and return an error if the device
        //   limit would otherwise be exceeded.
        let buffer_mem = unsafe {
            device.logical_device.allocate_memory(&alloc_info, None)
                .map_err(|err| err.to_string())?
        };

        // SAFETY:
        // VUID-vkBindBufferMemory-buffer-07459:
        //   We just created the buffer so it cannot yet have been bound.
        // VUID-vkBindBufferMemory-buffer-01030:
        //   We do not use sparse memory.
        // VUID-vkBindBufferMemory-memoryOffset-01031:
        //   Offset is zero and size is non-zero.
        // VUID-vkBindBufferMemory-memory-01035:
        // VUID-vkBindBufferMemory-size-01037:
        //   find_memory_type guarantees the returned memory type meets the
        //   requirements of the buffer.
        // VUID-vkBindBufferMemory-memoryOffset-01036:
        //   Offset is zero and therefore satisfies all possible alignments.
        // All other constraints are related to extensions that are not in use.
        unsafe {
            device.logical_device.bind_buffer_memory(buffer, buffer_mem, 0)
                .map_err(|err| err.to_string())?;
        }

        let mut buf = Self {
            handle: buffer,
            mem: buffer_mem,
            size,
            usage_flags: usage,
            is_map_in_use: RefCell::new(false),
            persistent_map: None,
            is_destroyed: false,
        };
        if props.contains(vk::MemoryPropertyFlags::HOST_VISIBLE) {
            // SAFETY:
            // VUID-vkMapMemory-memory-00678:
            //   We just created the buffer so it cannot yet have been mapped.
            // VUID-vkMapMemory-offset-00679:
            //   Offset is zero.
            // VUID-vkMapMemory-size-00680:
            // VUID-vkMapMemory-size-00681:
            //   Size is VK_WHOLE_SIZE.
            // VUID-vkMapMemory-memory-00682:
            //   find_memory_type will not return a memory type without
            //   HOST_VISIBLE_BIT.
            // VUID-vkMapMemory-memory-00683:
            //   We only use one instance.
            // VUID-vkMapMemory-flags-09568:
            //   We do not pass any flags.
            let buffer_ptr = unsafe {
                device.logical_device.map_memory(
                    buf.mem,
                    0,
                    vk::WHOLE_SIZE,
                    vk::MemoryMapFlags::empty(),
                )
                    .map_err(|err| err.to_string())?
            };
            buf.persistent_map = Some(buffer_ptr.cast());
        }

        Ok(buf)
    }

    /// SAFETY: The returned handle must not outlive `self`.
    pub(crate) unsafe fn get_handle(&self) -> vk::Buffer {
        self.handle
    }
    
    pub(crate) fn len(&self) -> vk::DeviceSize {
        self.size
    }

    pub(crate) fn map(
        &mut self,
        device: &VulkanDevice,
        offset: vk::DeviceSize,
        size: vk::DeviceSize,
        flags: vk::MemoryMapFlags,
    ) -> VkResult<MappedBuffer> {
        assert!(!self.handle.is_null());
        assert!(!*self.is_map_in_use.borrow(), "Buffer is already mapped elsewhere");
        assert!(offset < self.size);
        assert!(!flags.contains(vk::MemoryMapFlags::PLACED_EXT));

        let real_size = if size == vk::WHOLE_SIZE {
            self.size - offset
        } else {
            size
        };
        assert!(offset + real_size <= self.size);

        *self.is_map_in_use.borrow_mut() = true;

        let buffer_ptr = if let Some(persistent_map) = self.persistent_map {
            // if it's persistently mapped just use the existing pointer

            // SAFETY: We check above whether the offset falls within the buffer's
            // capacity so we can't return a pointer beyond its end.
            unsafe { persistent_map.byte_offset(offset as isize) }
        } else {
            // otherwise we have to call vkMapMemory to obtain a new pointer

            // SAFETY:
            // VUID-vkMapMemory-memory-00678:
            //   We explicitly track whether the map is in use and in turn
            //   whether it is currently mapped when persistent mapping is off.
            // VUID-vkMapMemory-offset-00679:
            //   We assert offset < size above.
            // VUID-vkMapMemory-size-00680:
            // VUID-vkMapMemory-size-00681:
            //    We assert above that the effective size is less than the size
            //   of allocation minus offset.
            // VUID-vkMapMemory-memory-00682:
            //   find_memory_type will not return a memory type without
            //   HOST_VISIBLE_BIT.
            // VUID-vkMapMemory-memory-00683:
            //   We only use one instance.
            // VUID-vkMapMemory-flags-09568:
            //   We assert above that flags does not contain PLACED_BIT_EXT.
            unsafe {
                device.logical_device.map_memory(self.mem, offset, size, flags)?.cast()
            }
        };

        // SAFETY: Attempting to create a map larger than the allocation will
        // trigger an assertion failure above, so the slice will not overrun the
        // mapped section of memory. The one edge case is when size is
        // VK_WHOLE_SIZE which we handle above by deriving the actual size of
        // the map and using that as the slice size instead.
        unsafe {
            Ok(MappedBuffer {
                buffer: self,
                map: slice::from_raw_parts_mut(buffer_ptr, real_size as usize),
            })
        }
    }

    pub(crate) fn copy_from(
        &mut self,
        device: &VulkanDevice,
        src_buf: &VulkanBuffer,
        src_off: vk::DeviceSize,
        cmd_buf: &CommandBufferInfo,
        dst_off: vk::DeviceSize,
        size: vk::DeviceSize,
    ) {
        assert!(!src_buf.handle.is_null());
        assert!(!self.handle.is_null());
        //TODO: ensure we don't try to copy between devices
        assert_ne!(src_buf.handle, self.handle, "Cannot copy buffer into itself");
        assert!(src_buf.usage_flags.contains(vk::BufferUsageFlags::TRANSFER_SRC));
        assert!(self.usage_flags.contains(vk::BufferUsageFlags::TRANSFER_DST));
        assert!(src_off <= src_buf.size);
        assert!(dst_off <= self.size);
        let real_size = if size == vk::WHOLE_SIZE {
            src_buf.size - src_off
        } else {
            size
        };
        assert!(real_size <= src_buf.size);
        assert!(real_size <= self.size);

        let copy_region = vk::BufferCopy::default()
            .src_offset(src_off)
            .dst_offset(dst_off)
            .size(real_size);
        // SAFETY:
        // VUID-vkCmdCopyBuffer-srcOffset-00113:
        // VUID-vkCmdCopyBuffer-dstOffset-00114:
        // VUID-vkCmdCopyBuffer-size-00115:
        // VUID-vkCmdCopyBuffer-size-00116:
        // VUID-vkCmdCopyBuffer-srcBuffer-00118:
        // VUID-vkCmdCopyBuffer-dstBuffer-00120:
        //   Explicitly checked by assertions above.
        // VUID-vkCmdCopyBuffer-pRegions-00117:
        //   We only copy a single region at a time.
        // VUID-vkCmdCopyBuffer-srcBuffer-00119:
        // VUID-vkCmdCopyBuffer-dstBuffer-00121:
        //   We only ever bind to a single memory object.
        // All other constraints are related to extensions that are not in use.
        unsafe {
            device.logical_device
                .cmd_copy_buffer(cmd_buf.get_handle(), src_buf.handle, self.handle, &[copy_region]);
        }
    }

    pub(crate) fn write<T: Copy>(
        &mut self,
        device: &VulkanDevice,
        src: &[T],
        dst_offset: vk::DeviceSize,
    ) -> VkResult<()> {
        assert_eq!(
            self.size % size_of::<T>() as vk::DeviceSize,
            0,
            "Element type size is not divisor of buffer size",
        );
        assert!(
            dst_offset + size_of_val(src) as vk::DeviceSize <= self.size,
            "Invalid offset/size params for buffer write",
        );
        assert!(!*self.is_map_in_use.borrow(), "Buffer is currently mapped elsewhere");

        let mut mapped = self.map(
            device,
            dst_offset,
            size_of_val(src) as vk::DeviceSize,
            vk::MemoryMapFlags::empty(),
        )?;
        mapped.as_slice_mut().copy_from_slice(src);

        Ok(())
    }

    pub(crate) fn destroy(mut self, device: &VulkanDevice) {
        assert!(!self.handle.is_null());
        assert!(!*self.is_map_in_use.borrow());

        if self.persistent_map.is_some() {
            self.persistent_map = None;
            unsafe { device.logical_device.unmap_memory(self.mem) };
        }

        // SAFETY:
        // VUID-vkFreeMemory-memory-00677:
        //   TODO: Not provable at this time.
        unsafe { device.logical_device.free_memory(self.mem, None); }
        // SAFETY:
        // VUID-vkDestroyBuffer-buffer-00922:
        //   TODO: Not provable at this time.
        // VUID-vkDestroyBuffer-buffer-00923:
        //   We do not use explicit allocation callbacks.
        // VUID-vkDestroyBuffer-buffer-00924:
        //   We do not use explicit allocation callbacks and we pass None here.
        unsafe { device.logical_device.destroy_buffer(self.handle, None); }
        ALLOCATION_COUNT.fetch_sub(1, Ordering::Relaxed);
        
        self.is_destroyed = true;
    }
}

impl Drop for VulkanBuffer {
    fn drop(&mut self) {
        if cfg!(debug_assertions) && !self.is_destroyed {
            panic!("Vulkan buffer was dropped without being destroyed!");
        }
    }
}
