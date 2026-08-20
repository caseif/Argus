use argus_scripting_bind::script_bind;
use argus_util::dirtiable::{Dirtiable, ValueAndDirtyFlag};
use argus_util::math::Vector3f;
use crate::common::Transform2d;

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[script_bind]
pub enum Light2dType {
    Point = 0,
}

#[derive(Clone, Copy, Debug)]
#[script_bind]
pub struct Light2dProperties {
    /// The type of light.
    pub ty: Light2dType,
    /// The RGB color of the light as normalized float values.
    pub color: Vector3f,
    /// Whether the light can be occluded by objects.
    pub is_occludable: bool,
    /// The absolute intensity of the light between 0 and 1.
    pub intensity: f32,
    /// How sharply the light drops off. Values above 1 will result in a slower
    /// falloff close to the light and a steeper one further away from it, while
    /// values between 0 and 1 will 
    pub falloff_gradient: f32,
    /// The distance (outside the falloff buffer) over which the light intensity
    /// falls to zero.
    pub falloff_distance: f32,
    /// The distance at which the light will begin to fall off. At shorter
    /// distances the light will be at full intensity.
    pub falloff_buffer: f32,
    /// How sharply the light drops off when occluded by an object. See
    /// `falloff_gradient` for more information.
    pub shadow_falloff_gradient: f32,
    /// The distance over which the light intensity falls to zero after being
    /// occluded by an object.
    pub shadow_falloff_distance: f32,
}

impl Default for Light2dProperties {
    fn default() -> Self {
        Self {
            ty: Light2dType::Point,
            color: Default::default(),
            is_occludable: false,
            intensity: 1.0,
            falloff_gradient: 1.0,
            falloff_distance: 1.0,
            falloff_buffer: 0.0,
            shadow_falloff_gradient: 1.0,
            shadow_falloff_distance: 1.0,
        }
    }
}

pub struct RenderLight2d {
    properties: Light2dProperties,
    transform: Dirtiable<Transform2d>,
}

impl RenderLight2d {
    #[must_use]
    pub(crate) fn new(
        properties: Light2dProperties,
        transform: Transform2d
    ) -> RenderLight2d {
        Self {
            properties,
            transform: Dirtiable::new(transform),
        }
    }

    #[must_use]
    pub fn get_properties(&self) -> &Light2dProperties {
        &self.properties
    }

    #[must_use]
    pub fn get_properties_mut(&mut self) -> &mut Light2dProperties {
        &mut self.properties
    }

    pub fn set_properties(&mut self, params: Light2dProperties) {
        self.properties = params;
    }

    #[must_use]
    pub fn get_transform(&mut self) -> ValueAndDirtyFlag<Transform2d> {
        self.transform.read()
    }
    
    #[must_use]
    pub fn peek_transform(&self) -> &Transform2d {
        self.transform.peek_ref().value
    }

    pub fn set_transform(&mut self, transform: Transform2d) {
        if &transform == self.transform.peek_ref().value {
            return;
        }
        self.transform.set(transform);
    }

    pub fn to_shader_repr(&self) -> Std140Light2D {
        let pos = &self.peek_transform().translation;
        let props = self.get_properties();
        let color = props.color;
        Std140Light2D {
            color: [color.x, color.y, color.z, 1.0],
            position: [pos.x, pos.y, 0.0, 1.0],
            intensity: props.intensity,
            falloff_gradient: props.falloff_gradient,
            falloff_distance: props.falloff_distance,
            falloff_buffer: props.falloff_buffer,
            shadow_falloff_gradient: props.shadow_falloff_gradient,
            shadow_falloff_distance: props.shadow_falloff_distance,
            ty: props.ty as i32,
            is_occludable: if props.is_occludable { 1 } else { 0 },
            //unused: 0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Default)]
pub struct Std140Light2D {
    // offset 0
    pub color: [f32; 4],
    // offset 16
    pub position: [f32; 4],
    // offset 32
    pub intensity: f32,
    // offset 36
    pub falloff_gradient: f32,
    // offset 40
    pub falloff_distance: f32,
    // offset 44
    pub falloff_buffer: f32,
    // offset 48
    pub shadow_falloff_gradient: f32,
    // offset 52
    pub shadow_falloff_distance: f32,
    // offset 56
    pub ty: i32,
    // offset 60
    pub is_occludable: u32,
}
