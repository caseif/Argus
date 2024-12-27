use std::cell::RefCell;
use std::ops::{Deref, Mul, MulAssign};
use std::sync::Arc;
use std::sync::atomic::{AtomicU16, Ordering};
use argus_scripting_bind::script_bind;
use lowlevel_rustabi::argus::lowlevel::{Vector2f, Vector4f};
use parking_lot::{RwLock, RwLockReadGuard, RwLockUpgradableReadGuard, RwLockWriteGuard};

#[derive(Debug)]
#[script_bind(rename = "RsTransform2d")]
pub struct Transform2d {
    pub translation: Vector2f,
    pub scale: Vector2f,
    pub rotation: f32,

    version: Option<Arc<AtomicU16>>,
    matrices: RwLock<Option<TransformMatrixCache>>,
}

#[derive(Clone, Copy, Debug, Default)]
struct TransformMatrixCache {
    translation_matrix: Matrix4x4,
    rotation_matrix: Matrix4x4,
    scale_matrix: Matrix4x4,
    full_matrix: Option<(Vector2f, Matrix4x4)>,
}

impl Clone for Transform2d {
    fn clone(&self) -> Self {
        Self {
            translation: self.translation,
            scale: self.scale,
            rotation: self.rotation,
            version: self.version.clone(),
            matrices: RwLock::new(self.matrices.read().clone()),
        }
    }
}

impl PartialEq for Transform2d {
    fn eq(&self, other: &Self) -> bool {
        self.translation == other.translation &&
            self.scale == other.scale &&
            self.rotation == other.rotation
    }
}

#[script_bind]
impl Transform2d {
    #[script_bind]
    pub fn new(translation: Vector2f, scale: Vector2f, rotation: f32) -> Self {
        Self {
            translation,
            scale,
            rotation,
            version: None,
            matrices: Default::default(),
        }
    }
}

#[derive(Clone, Copy, Debug, Default)]
pub struct Matrix4x4 {
    pub cells: [f32; 16],
}

impl Matrix4x4 {
    pub fn identity() -> Self {
        Self {
            cells: [
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
            ]
        }
    }
}

#[script_bind]
impl Default for Transform2d {
    #[script_bind]
    fn default() -> Self {
        Self::new(
            Vector2f { x: 0.0, y: 0.0 },
            Vector2f { x: 1.0, y: 1.0 },
            0.0,
        )
    }
}

#[script_bind]
impl Transform2d {
    #[must_use]
    #[script_bind]
    pub fn get_translation(&self) -> Vector2f {
        self.translation
    }

    #[script_bind]
    pub fn add_translation(&mut self, x: f32, y: f32) {
        self.translation += (x, y);
        *self.matrices.write() = None;
        self.bump_version();
    }

    #[must_use]
    #[script_bind]
    pub fn get_scale(&self) -> Vector2f {
        self.scale
    }

    #[script_bind]
    pub fn set_scale(&mut self, x: f32, y: f32) {
        self.scale = Vector2f { x, y };
        *self.matrices.write() = None;
        self.bump_version();
    }

    #[must_use]
    #[script_bind]
    pub fn get_rotation(&self) -> f32 {
        self.rotation
    }

    #[script_bind]
    pub fn add_rotation(&mut self, rads: f32) {
        self.rotation += rads;
        *self.matrices.write() = None;
        self.bump_version();
    }

    #[must_use]
    #[script_bind]
    pub fn inverse(&self) -> Self {
        Self::new(-self.translation, self.scale, -self.rotation)
    }

    #[must_use]
    pub fn as_matrix(&self, anchor_point: Vector2f) -> Matrix4x4 {
        let guard = self.compute_matrices(anchor_point);
        guard.expect("Transform matrix cache is missing")
            .full_matrix.expect("Transform cached full matrix is missing").1
    }

    #[must_use]
    pub fn get_translation_matrix(&self) -> Matrix4x4 {
        let guard = self.compute_aux_matrices();
        guard.expect("Transform matrix cache is missing").translation_matrix
    }

    #[must_use]
    pub fn get_rotation_matrix(&self) -> Matrix4x4 {
        let guard = self.compute_aux_matrices();
        guard.expect("Transform matrix cache is missing").rotation_matrix
    }

    #[must_use]
    pub fn get_scale_matrix(&self) -> Matrix4x4 {
        let guard = self.compute_aux_matrices();
        guard.expect("Transform matrix cache is missing").scale_matrix
    }

    fn bump_version(&mut self) {
        if let Some(version) = &self.version {
            version.fetch_add(1, Ordering::Relaxed);
        }
    }

    fn compute_aux_matrices(&self) -> RwLockReadGuard<Option<TransformMatrixCache>> {
        let guard = self.matrices.upgradable_read();
        RwLockUpgradableReadGuard::downgrade(
            self.compute_aux_matrices_with_guard(guard)
        )
    }

    fn compute_aux_matrices_with_guard<'a>(
        &self,
        guard: RwLockUpgradableReadGuard<'a, Option<TransformMatrixCache>>
    ) -> RwLockUpgradableReadGuard<'a, Option<TransformMatrixCache>> {
        if guard.is_some() {
            return guard;
        }

        let cos_rot = self.rotation.cos();
        let sin_rot = self.rotation.sin();

        let mut mats_write = RwLockUpgradableReadGuard::upgrade(guard);
        *mats_write = Some(TransformMatrixCache {
            translation_matrix: Matrix4x4::from_row_major([
                    1.0, 0.0, 0.0, self.translation.x,
                    0.0, 1.0, 0.0, self.translation.y,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0,
            ]),
            rotation_matrix: Matrix4x4::from_row_major([
                    cos_rot, -sin_rot, 0.0, 0.0,
                    sin_rot, cos_rot, 0.0, 0.0,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0,
            ]),
            scale_matrix: Matrix4x4::from_row_major([
                self.scale.x, 0.0, 0.0, 0.0,
                0.0, self.scale.y, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
            ]),
            full_matrix: None,
        });
        RwLockWriteGuard::downgrade_to_upgradable(mats_write)
    }

    fn compute_transform_matrix<'a>(
        &self,
        anchor_point: Vector2f,
        mut guard: RwLockWriteGuard<'a, Option<TransformMatrixCache>>
    ) -> RwLockWriteGuard<'a, Option<TransformMatrixCache>> {
        //auto cur_translation = transform.get_translation();

        //UNUSED(anchor_point);
        let anchor_mat_1 = Matrix4x4::from_row_major([
                1.0, 0.0, 0.0, -anchor_point.x,
                0.0, 1.0, 0.0, -anchor_point.y,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
        ]);
        let anchor_mat_2 = Matrix4x4::from_row_major([
                1.0, 0.0, 0.0, anchor_point.x,
                0.0, 1.0, 0.0, anchor_point.y,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
        ]);
        guard.as_mut().expect("Transform matrix was computed before aux matrices").full_matrix =
            Some(
                (
                    anchor_point,
                    guard.unwrap().translation_matrix *
                        anchor_mat_2 *
                        guard.unwrap().rotation_matrix *
                        guard.unwrap().scale_matrix *
                        anchor_mat_1,
                )
            );
        guard
    }

    fn compute_matrices(&self, anchor_point: Vector2f)
        -> RwLockReadGuard<Option<TransformMatrixCache>> {
        let mut guard = self.matrices.upgradable_read();
        let (compute_aux, compute_full) = match guard.deref() {
            Some(cache) => {
                (false, cache.full_matrix.map(|(p, _)| p == anchor_point).unwrap_or(true))
            },
            None => (true, true),
        };
        if compute_aux {
            guard = self.compute_aux_matrices_with_guard(guard);
        }
        if compute_full {
            RwLockWriteGuard::downgrade(
                self.compute_transform_matrix(
                    anchor_point,
                    RwLockUpgradableReadGuard::upgrade(guard)
                )
            )
        } else {
            RwLockUpgradableReadGuard::downgrade(guard)
        }
    }
}

impl Matrix4x4 {
    #[must_use]
    pub fn from_row_major(vals: [f32; 16]) -> Self {
        Self {
            cells: [
                vals[0], vals[4], vals[8], vals[12],
                vals[1], vals[5], vals[9], vals[13],
                vals[2], vals[6], vals[10], vals[14],
                vals[3], vals[7], vals[11], vals[15],
            ]
        }
    }

    #[must_use]
    pub fn get(&self, row: usize, col: usize) -> f32 {
        self.cells[col * 4 + row]
    }

    #[must_use]
    pub fn get_mut(&mut self, row: usize, col: usize) -> &mut f32 {
        &mut self.cells[col * 4 + row]
    }
}

impl Mul<Self> for &Matrix4x4 {
    type Output = Matrix4x4;

    #[must_use]
    fn mul(self, other: Self) -> Self::Output {
        let mut res: [f32; 16] = Default::default();

        // naive implementation
        for i in 0..4 {
            for j in 0..4 {
                res[j * 4 + i] = 0.0;
                for k in 0..4 {
                    res[j * 4 + i] += self.get(i, k) * other.get(k, j);
                }
            }
        }

        Matrix4x4 { cells: res }
    }
}

impl Mul<Self> for Matrix4x4 {
    type Output = Matrix4x4;

    #[must_use]
    fn mul(self, other: Self) -> Self::Output {
        &self * &other
    }
}

impl MulAssign<Self> for Matrix4x4 {
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl Mul<Vector4f> for &Matrix4x4 {
    type Output = Vector4f;

    #[must_use]
    fn mul(self, vector: Vector4f) -> Self::Output {
        Vector4f {
            x: self.get(0, 0) * vector.x +
                self.get(0, 1) * vector.y +
                self.get(0, 2) * vector.z +
                self.get(0, 3) * vector.w,
            y: self.get(1, 0) * vector.x +
                self.get(1, 1) * vector.y +
                self.get(1, 2) * vector.z +
                self.get(1, 3) * vector.w,
            z: self.get(2, 0) * vector.x +
                self.get(2, 1) * vector.y +
                self.get(2, 2) * vector.z +
                self.get(2, 3) * vector.w,
            w: self.get(3, 0) * vector.x +
                self.get(3, 1) * vector.y +
                self.get(3, 2) * vector.z +
                self.get(3, 3) * vector.w
        }
    }
}

impl Mul<Vector4f> for Matrix4x4 {
    type Output = Vector4f;

    #[must_use]
    fn mul(self, vector: Vector4f) -> Self::Output {
        &self * vector
    }
}
