use std::ops::{Mul, MulAssign};
use crate::math::{Vector4f};

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

    #[must_use]
    pub fn inverse(&self) -> Option<Matrix4x4> {
        if self.cells[3] == 0.0 && self.cells[7] == 0.0 &&
            self.cells[11] == 0.0 && self.cells[15] == 1.0 {
            // matrix is affine
            let det = self.cells[0] * (self.cells[5] * self.cells[10] - self.cells[9] * self.cells[6]) -
                self.cells[4] * (self.cells[1] * self.cells[10] - self.cells[9] * self.cells[2]) +
                self.cells[8] * (self.cells[1] * self.cells[6] - self.cells[5] * self.cells[2]);
            if det == 0.0 {
                return None;
            }

            let mat_3x3_inv: [f32; 9] = [
                (self.cells[5] * self.cells[10] - self.cells[6] * self.cells[9]) / det,
                (self.cells[2] * self.cells[9] - self.cells[1] * self.cells[10]) / det,
                (self.cells[1] * self.cells[6] - self.cells[2] * self.cells[5]) / det,
                (self.cells[6] * self.cells[8] - self.cells[4] * self.cells[10]) / det,
                (self.cells[0] * self.cells[10] - self.cells[2] * self.cells[8]) / det,
                (self.cells[1] * self.cells[8] - self.cells[0] * self.cells[9]) / det,
                (self.cells[4] * self.cells[9] - self.cells[5] * self.cells[8]) / det,
                (self.cells[2] * self.cells[4] - self.cells[0] * self.cells[6]) / det,
                (self.cells[0] * self.cells[5] - self.cells[1] * self.cells[4]) / det,
            ];
            let b: [f32; 3] = [
                -(mat_3x3_inv[0] * self.cells[12] +
                    mat_3x3_inv[3] * self.cells[13] +
                    mat_3x3_inv[6] * self.cells[14]),
                -(mat_3x3_inv[1] * self.cells[12] +
                    mat_3x3_inv[4] * self.cells[13] +
                    mat_3x3_inv[7] * self.cells[14]),
                -(mat_3x3_inv[2] * self.cells[12] +
                    mat_3x3_inv[5] * self.cells[13] +
                    mat_3x3_inv[8] * self.cells[14]),
            ];

            Some(Matrix4x4 {
                cells: [
                    mat_3x3_inv[0],
                    mat_3x3_inv[1],
                    mat_3x3_inv[2],
                    0.0,
                    mat_3x3_inv[3],
                    mat_3x3_inv[4],
                    mat_3x3_inv[5],
                    0.0,
                    mat_3x3_inv[6],
                    mat_3x3_inv[7],
                    mat_3x3_inv[8],
                    0.0,
                    b[0],
                    b[1],
                    b[2],
                    1.0,
                ],
            })
        } else {
            // non-affine
            todo!()
        }
    }
}

impl Mul<Self> for &Matrix4x4 {
    type Output = Matrix4x4;

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

    fn mul(self, other: Self) -> Self::Output {
        Mul::<&Matrix4x4>::mul(&self, &other)
    }
}

impl MulAssign<Self> for Matrix4x4 {
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl Mul<Vector4f> for &Matrix4x4 {
    type Output = Vector4f;

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

    fn mul(self, vector: Vector4f) -> Self::Output {
        Mul::<Vector4f>::mul(&self, vector)
    }
}
