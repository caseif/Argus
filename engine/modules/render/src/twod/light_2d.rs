use argus_scripting_bind::script_bind;
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
    /// Higher values will result in a steeper falloff gradient.
    pub falloff_gradient: f32,
    /// Higher values will increase the distance over which the light falls off.
    pub falloff_multiplier: f32,
    /// Higher values will increase the distance before the light starts to fall
    /// off.
    pub falloff_buffer: f32,
    /// Higher values will result in a steeper falloff gradient in shadows.
    pub shadow_falloff_gradient: f32,
    /// Higher values will increase the distance over which light falls off in a
    /// shadow.
    pub shadow_falloff_multiplier: f32,
}

pub struct RenderLight2d {
    properties: Light2dProperties,
    transform: Transform2d,
}

impl RenderLight2d {
    #[must_use]
    pub(crate) fn new(
        properties: Light2dProperties,
        transform: Transform2d
    ) -> RenderLight2d {
        Self {
            properties: properties,
            transform,
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
    pub fn get_transform(&self) -> &Transform2d {
        &self.transform
    }

    pub fn set_transform(&mut self, transform: Transform2d) {
        if transform == self.transform {
            return;
        }
        self.transform = transform;
    }
}
