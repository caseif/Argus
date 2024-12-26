use crate::common::Vertex2d;

pub struct RenderPrimitive2d {
    pub vertices: Vec<Vertex2d>,
}

impl RenderPrimitive2d {
    pub fn new(vertices: Vec<Vertex2d>) -> Self {
        Self { vertices }
    }

    #[must_use]
    pub fn get_vertices(&self) -> &Vec<Vertex2d> {
        &self.vertices
    }
}
