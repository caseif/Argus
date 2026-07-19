use std::ops::Index;
use std::slice;
use argus_shadertools::ShaderReflectionInfo;

use crate::vk;
use crate::vk::Wrapper;

pub use ash::vk::DescriptorType;

const INITIAL_VIEWPORT_CAP: u32 = 2;
const INITIAL_BUCKET_CAP: u32 = 64;
const SAMPLERS_PER_BUCKET: u32 = 1;
const UBOS_PER_BUCKET: u32 = 3;
const INITIAL_DS_COUNT: u32 =
    INITIAL_VIEWPORT_CAP *
    INITIAL_BUCKET_CAP *
    (vk::MAX_FRAMES_IN_FLIGHT as u32);
const INITIAL_UBO_COUNT: u32 = INITIAL_DS_COUNT * UBOS_PER_BUCKET;
const INITIAL_SAMPLER_COUNT: u32 = INITIAL_DS_COUNT * SAMPLERS_PER_BUCKET;

pub struct DescriptorBufferInfo<'ctx> {
    pub(crate) buffer: &'ctx vk::Buffer<'ctx>,
    pub(crate) offset: vk::DeviceSize,
    pub(crate) range: vk::DeviceSize,
}

impl<'ctx> DescriptorBufferInfo<'ctx> {
    pub fn new(buffer: &'ctx vk::Buffer<'ctx>) -> Self {
        Self {
            buffer,
            offset: 0,
            range: 0,
        }
    }

    pub fn offset(mut self, offset: vk::DeviceSize) -> Self {
        self.offset = offset;
        self
    }

    pub fn range(mut self, range: vk::DeviceSize) -> Self {
        self.range = range;
        self
    }
}

#[derive(Default)]
pub struct DescriptorImageInfo<'ctx> {
    pub(crate) image_layout: Option<vk::ImageLayout>,
    pub(crate) image_view: Option<&'ctx vk::ImageView<'ctx>>,
    pub(crate) sampler: Option<&'ctx vk::Sampler<'ctx>>,
}

impl<'ctx> DescriptorImageInfo<'ctx> {
    pub fn image_layout(mut self, layout: vk::ImageLayout) -> Self {
        self.image_layout = Some(layout);
        self
    }

    pub fn image_view(mut self, view: &'ctx vk::ImageView<'ctx>) -> Self {
        self.image_view = Some(view);
        self
    }

    pub fn sampler(mut self, sampler: &'ctx vk::Sampler<'ctx>) -> Self {
        self.sampler = Some(sampler);
        self
    }
}

#[derive(Default)]
pub struct WriteDescriptorSet<'ctx> {
    pub(crate) dst_set: Option<&'ctx DescriptorSet<'ctx>>,
    pub(crate) dst_binding: Option<u32>,
    pub(crate) dst_array_element: Option<u32>,
    pub(crate) descriptor_type: Option<ash::vk::DescriptorType>,
    pub(crate) descriptor_count: Option<u32>,
    pub(crate) buffer_info: Option<Vec<DescriptorBufferInfo<'ctx>>>,
    pub(crate) image_infos: Option<Vec<DescriptorImageInfo<'ctx>>>,
}

impl<'ctx> WriteDescriptorSet<'ctx> {
    pub fn dst_set(mut self, dst_set: &'ctx vk::DescriptorSet<'ctx>) -> Self {
        self.dst_set = Some(dst_set);
        self
    }

    pub fn dst_binding(mut self, dst_binding: u32) -> Self {
        self.dst_binding = Some(dst_binding);
        self
    }

    pub fn dst_array_element(mut self, dst_array_element: u32) -> Self {
        self.dst_array_element = Some(dst_array_element);
        self
    }

    pub fn descriptor_type(mut self, descriptor_type: ash::vk::DescriptorType) -> Self {
        self.descriptor_type = Some(descriptor_type);
        self
    }

    pub fn descriptor_count(mut self, descriptor_count: u32) -> Self {
        self.descriptor_count = Some(descriptor_count);
        self
    }

    pub fn buffer_info(mut self, buffer_info: impl Into<Vec<DescriptorBufferInfo<'ctx>>>)
                       -> Self {
        self.buffer_info = Some(buffer_info.into());
        self
    }

    pub fn image_info(mut self, image_info: impl Into<Vec<DescriptorImageInfo<'ctx>>>) -> Self {
        self.image_infos = Some(image_info.into());
        self
    }

    pub(crate) fn prepare(self) -> PreparedWriteDescriptorSet<'ctx> {
        PreparedWriteDescriptorSet {
            dst_set: self.dst_set.unwrap(),
            dst_binding: self.dst_binding.unwrap(),
            dst_array_element: self.dst_array_element.unwrap(),
            descriptor_type: self.descriptor_type.unwrap(),
            descriptor_count: self.descriptor_count.unwrap(),
            buffer_infos: self.buffer_info
                .map(|buffer_infos| buffer_infos.into_iter().map(|buf_info|
                    ash::vk::DescriptorBufferInfo::default()
                        .buffer(unsafe { buf_info.buffer.get_underlying() })
                        .offset(buf_info.offset)
                        .range(buf_info.range)
                ).collect()),
            image_infos: self.image_infos
                .map(|image_infos| image_infos.into_iter().map(|img_info|
                    ash::vk::DescriptorImageInfo::default()
                        .image_layout(img_info.image_layout.unwrap())
                        .image_view(unsafe { img_info.image_view.unwrap().get_underlying() })
                        .sampler(unsafe { img_info.sampler.unwrap().get_underlying() })
            ).collect()),
        }
    }
}

pub(crate) struct PreparedWriteDescriptorSet<'ctx> {
    pub(crate) dst_set: &'ctx DescriptorSet<'ctx>,
    pub(crate) dst_binding: u32,
    pub(crate) dst_array_element: u32,
    pub(crate) descriptor_type: ash::vk::DescriptorType,
    pub(crate) descriptor_count: u32,
    pub(crate) buffer_infos: Option<Vec<ash::vk::DescriptorBufferInfo>>,
    pub(crate) image_infos: Option<Vec<ash::vk::DescriptorImageInfo>>,
}

impl<'ctx> PreparedWriteDescriptorSet<'ctx> {
    pub(crate) fn create_underlying(&'_ self) -> ash::vk::WriteDescriptorSet<'_> {
        let mut vk_ds = ash::vk::WriteDescriptorSet::default()
            .dst_set(unsafe { self.dst_set.get_underlying() })
            .dst_binding(self.dst_binding)
            .dst_array_element(self.dst_array_element)
            .descriptor_type(self.descriptor_type)
            .descriptor_count(self.descriptor_count);

        if let Some(buffer_infos) = self.buffer_infos.as_ref() {
            vk_ds = vk_ds.buffer_info(buffer_infos);
        }
        if let Some(image_infos) = self.image_infos.as_ref() {
            vk_ds = vk_ds.image_info(image_infos);
        }

        vk_ds
    }
}

fn create_ubo_bindings<'a>(shader_refl: &ShaderReflectionInfo)
    -> Vec<ash::vk::DescriptorSetLayoutBinding<'a>> {
    shader_refl.get_ubo_bindings().iter()
        .map(|(_, ubo)| {
            ash::vk::DescriptorSetLayoutBinding::default()
                .binding(*ubo)
                .descriptor_type(ash::vk::DescriptorType::UNIFORM_BUFFER)
                .descriptor_count(1) //TODO: account for array UBOs
                .stage_flags(vk::ShaderStageFlags::ALL_GRAPHICS)
        })
        .collect()
}

fn create_sampler_binding<'a>() -> ash::vk::DescriptorSetLayoutBinding<'a> {
    ash::vk::DescriptorSetLayoutBinding::default()
        .binding(0) //TODO: pass actual value through reflection info
        .descriptor_type(ash::vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
        .descriptor_count(1)
        .stage_flags(vk::ShaderStageFlags::ALL_GRAPHICS)
}

pub struct DescriptorPool<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::DescriptorPool,
}

impl<'ctx> Wrapper for DescriptorPool<'ctx> {
    type Underlying = ash::vk::DescriptorPool;

    unsafe fn get_underlying(&self) -> ash::vk::DescriptorPool {
        self.underlying
    }
}

impl<'ctx> DescriptorPool<'ctx> {
    pub fn create(device: &'ctx vk::Device) -> Result<Self, String> {
        let ubo_pool_size = ash::vk::DescriptorPoolSize::default()
            .ty(ash::vk::DescriptorType::UNIFORM_BUFFER)
            .descriptor_count(INITIAL_UBO_COUNT);

        let sampler_pool_size = ash::vk::DescriptorPoolSize::default()
            .ty(ash::vk::DescriptorType::COMBINED_IMAGE_SAMPLER)
            .descriptor_count(INITIAL_SAMPLER_COUNT);

        let pool_sizes = [ubo_pool_size, sampler_pool_size];

        let desc_pool_info = ash::vk::DescriptorPoolCreateInfo::default()
            .pool_sizes(&pool_sizes)
            .max_sets(INITIAL_DS_COUNT)
            .flags(ash::vk::DescriptorPoolCreateFlags::FREE_DESCRIPTOR_SET);

        let pool = unsafe {
            device.get_underlying().create_descriptor_pool(&desc_pool_info, None)
                .map_err(|err| err.to_string())?
        };
        Ok(Self {
            device,
            underlying: pool,
        })
    }

    pub fn destroy(self) {
        unsafe {
            self.device.get_underlying().destroy_descriptor_pool(self.underlying, None);
        }
    }
}

pub struct DescriptorSetLayout<'ctx> {
    pub device: &'ctx vk::Device<'ctx>,
    pub underlying: ash::vk::DescriptorSetLayout,
}

impl<'ctx> Wrapper for DescriptorSetLayout<'ctx> {
    type Underlying = ash::vk::DescriptorSetLayout;

    unsafe fn get_underlying(&self) -> ash::vk::DescriptorSetLayout {
        self.underlying
    }
}

impl<'ctx> DescriptorSetLayout<'ctx> {
    pub fn create(
        device: &'ctx vk::Device,
        shader_refl: &ShaderReflectionInfo
    ) -> Result<DescriptorSetLayout<'ctx>, String> {
        let ubo_bindings = create_ubo_bindings(shader_refl);
        let sampler_binding = create_sampler_binding();
        let mut all_bindings = ubo_bindings.clone();
        all_bindings.push(sampler_binding);

        DescriptorSetLayout::create_from_bindings(device, all_bindings)
    }

    pub fn create_from_bindings(
        device: &'ctx vk::Device,
        bindings: Vec<ash::vk::DescriptorSetLayoutBinding<'ctx>>,
    ) -> Result<Self, String> {
        assert!(bindings.len() <= u32::MAX as usize, "Too many descriptor set layout bindings");

        let layout_info = ash::vk::DescriptorSetLayoutCreateInfo::default()
            .bindings(&bindings);

        let layout = unsafe {
            device.get_underlying().create_descriptor_set_layout(&layout_info, None)
                .map_err(|err| err.to_string())?
        };
        Ok(Self {
            device,
            underlying: layout,
        })
    }

    pub fn destroy(self) {
        unsafe { self.device.get_underlying().destroy_descriptor_set_layout(self.underlying, None); }
    }
}

pub struct DescriptorSetGroup<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    sets: Vec<DescriptorSet<'ctx>>,
    desc_pool: ash::vk::DescriptorPool, // used for validation only
}

impl<'ctx> DescriptorSetGroup<'ctx> {
    pub fn create(
        device: &'ctx vk::Device<'ctx>,
        pool: &DescriptorPool<'ctx>,
        shader_refl: &ShaderReflectionInfo,
    ) -> Result<DescriptorSetGroup<'ctx>, String> {
        //TODO: frames in flight
        let layout = DescriptorSetLayout::create(device, shader_refl)?;
        let layouts = [layout.underlying];

        let ds_info = ash::vk::DescriptorSetAllocateInfo::default()
            .descriptor_pool(pool.underlying)
            .set_layouts(&layouts);

        let sets = unsafe {
            device.get_underlying().allocate_descriptor_sets(&ds_info)
                .map_err(|err| err.to_string())?
        };
        Ok(Self {
            device,
            sets: sets.into_iter().map(|ds| DescriptorSet {
                device,
                underlying: ds,
            }).collect(),
            desc_pool: unsafe { pool.get_underlying() },
        })
    }

    pub fn destroy(self, desc_pool: &DescriptorPool<'ctx>) -> Result<(), String> {
        if unsafe { desc_pool.get_underlying() } != self.desc_pool {
            return Err(
                "Cannot use different descriptor pool to destroy descriptor sets".to_string()
            );
        }
        let underlying: Vec<_> = self.sets.iter().map(|ds| ds.underlying).collect();
        unsafe {
            self.device.get_underlying().free_descriptor_sets(desc_pool.get_underlying(), &underlying)
                .map_err(|err| err.to_string())
        }
    }

    pub fn iter(&self) -> slice::Iter<'_, DescriptorSet<'_>> {
        self.sets.iter()
    }
}

impl<'pool, 'ctx: 'pool> Iterator for DescriptorSetGroup<'ctx> {
    type Item = DescriptorSet<'ctx>;
    fn next(&mut self) -> Option<Self::Item> {
        self.sets.pop()
    }
}

impl<'ctx> ExactSizeIterator for DescriptorSetGroup<'ctx> {
    fn len(&self) -> usize {
        self.sets.len()
    }
}

impl<'ctx> Index<usize> for DescriptorSetGroup<'ctx> {
    type Output = DescriptorSet<'ctx>;
    fn index(&self, index: usize) -> &Self::Output {
        &self.sets[index]
    }
}

pub struct DescriptorSet<'ctx> {
    #[allow(dead_code)]
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::DescriptorSet,
}

impl<'ctx> vk::Wrapper for DescriptorSet<'ctx> {
    type Underlying = ash::vk::DescriptorSet;

    unsafe fn get_underlying(&self) -> ash::vk::DescriptorSet {
        self.underlying
    }
}
