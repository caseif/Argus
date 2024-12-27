pub trait Scene {
    #[must_use]
    fn get_type(&self) -> SceneType;
}

pub enum SceneType {
    TwoDim,
    ThreeDim,
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub(crate) enum SceneItemType {
    Group,
    Object,
}