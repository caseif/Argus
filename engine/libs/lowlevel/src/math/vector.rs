use std::ops::{Add, AddAssign, Mul, MulAssign, Sub, SubAssign};
use num_traits::Num;
use crate::math::matrix::Matrix4;

#[derive(Default)]
pub struct Vector4<T: Copy + Default + Num> {
    x: T,
    y: T,
    z: T,
    w: T
}

impl<T: Copy + Default + Num> Vector4<T> {
    pub fn new(x: T, y: T, z: T, w: T) -> Self {
        return Vector4 { x, y, z, w };
    }
}

impl<T: Copy + Default + Num> Add for Vector4<T> {
    type Output = Vector4<T>;

    fn add(self, rhs: Self) -> Self::Output {
        Vector4 {
            x: self.x + rhs.x,
            y: self.y + rhs.y,
            z: self.z + rhs.z,
            w: self.w + rhs.w
        }
    }
}

impl<T: Copy + Default + Num + AddAssign> AddAssign for Vector4<T> {
    fn add_assign(&mut self, rhs: Self) {
        self.x += rhs.x;
        self.y += rhs.y;
        self.z += rhs.z;
        self.w += rhs.w;
    }
}

impl<T: Copy + Default + Num> Sub for Vector4<T> {
    type Output = Vector4<T>;

    fn sub(self, rhs: Self) -> Self::Output {
        Vector4 {
            x: self.x - rhs.x,
            y: self.y - rhs.y,
            z: self.z - rhs.z,
            w: self.w - rhs.w
        }
    }
}

impl<T: Copy + Default + Num + SubAssign> SubAssign for Vector4<T> {
    fn sub_assign(&mut self, rhs: Self) {
        self.x -= rhs.x;
        self.y -= rhs.y;
        self.z -= rhs.z;
        self.w -= rhs.w;
    }
}

impl<T: Copy + Default + Num> Mul for Vector4<T> {
    type Output = Vector4<T>;

    fn mul(self, rhs: Self) -> Self::Output {
        Vector4 {
            x: self.x * rhs.x,
            y: self.y * rhs.y,
            z: self.z * rhs.z,
            w: self.w * rhs.w
        }
    }
}

impl<T: Copy + Default + Num + MulAssign> MulAssign for Vector4<T> {
    fn mul_assign(&mut self, rhs: Self) {
        self.x *= rhs.x;
        self.y *= rhs.y;
        self.z *= rhs.z;
        self.w *= rhs.w;
    }
}

impl<T: Copy + Default + Num> Mul<Matrix4> for Vector4<T> where f32: Mul<T, Output = T> {
    type Output = Vector4<T>;

    fn mul(self, mat: Matrix4) -> Self::Output {
        Vector4::<T> {
            x: (mat[0][0] * self.x + mat[0][1] * self.y + mat[0][2] * self.z + mat[0][3] * self.w),
            y: (mat[1][0] * self.x + mat[1][1] * self.y + mat[1][2] * self.z + mat[1][3] * self.w),
            z: (mat[2][0] * self.x + mat[2][1] * self.y + mat[2][2] * self.z + mat[2][3] * self.w),
            w: (mat[3][0] * self.x + mat[3][1] * self.y + mat[3][2] * self.z + mat[3][3] * self.w)
        }
    }
}

pub type Vector4f = Vector4<f32>;
pub type Vector4d = Vector4<f64>;
pub type Vector4i = Vector4<i32>;
pub type Vector4u = Vector4<u32>;

#[derive(Default)]
pub struct Vector3<T: Copy + Default + Num> {
    x: T,
    y: T,
    z: T
}

impl<T: Copy + Default + Num> Vector3<T> {
    pub fn new(x: T, y: T, z: T) -> Self {
        return Vector3 { x, y, z };
    }
}

impl<T: Copy + Default + Num> Add for Vector3<T> {
    type Output = Vector3<T>;

    fn add(self, rhs: Self) -> Self::Output {
        Vector3 {
            x: self.x + rhs.x,
            y: self.y + rhs.y,
            z: self.z + rhs.z
        }
    }
}

impl<T: Copy + Default + Num + AddAssign> AddAssign for Vector3<T> {
    fn add_assign(&mut self, rhs: Self) {
        self.x += rhs.x;
        self.y += rhs.y;
        self.z += rhs.z;
    }
}

impl<T: Copy + Default + Num> Sub for Vector3<T> {
    type Output = Vector3<T>;

    fn sub(self, rhs: Self) -> Self::Output {
        Vector3 {
            x: self.x - rhs.x,
            y: self.y - rhs.y,
            z: self.z - rhs.z
        }
    }
}

impl<T: Copy + Default + Num + SubAssign> SubAssign for Vector3<T> {
    fn sub_assign(&mut self, rhs: Self) {
        self.x -= rhs.x;
        self.y -= rhs.y;
        self.z -= rhs.z;
    }
}

impl<T: Copy + Default + Num> Mul for Vector3<T> {
    type Output = Vector3<T>;

    fn mul(self, rhs: Self) -> Self::Output {
        Vector3 {
            x: self.x * rhs.x,
            y: self.y * rhs.y,
            z: self.z * rhs.z
        }
    }
}

impl<T: Copy + Default + Num + MulAssign> MulAssign for Vector3<T> {
    fn mul_assign(&mut self, rhs: Self) {
        self.x *= rhs.x;
        self.y *= rhs.y;
        self.z *= rhs.z;
    }
}

pub type Vector3f = Vector3<f32>;
pub type Vector3d = Vector3<f64>;
pub type Vector3i = Vector3<i32>;
pub type Vector3u = Vector3<u32>;

#[derive(Default)]
pub struct Vector2<T: Copy + Default + Num> {
    x: T,
    y: T
}

impl<T: Copy + Default + Num> Vector2<T> {
    pub fn new(x: T, y: T) -> Self {
        return Vector2 { x, y };
    }
}

impl<T: Copy + Default + Num> Add for Vector2<T> {
    type Output = Vector2<T>;

    fn add(self, rhs: Self) -> Self::Output {
        Vector2 {
            x: self.x + rhs.x,
            y: self.y + rhs.y
        }
    }
}

impl<T: Copy + Default + Num + AddAssign> AddAssign for Vector2<T> {
    fn add_assign(&mut self, rhs: Self) {
        self.x += rhs.x;
        self.y += rhs.y;
    }
}

impl<T: Copy + Default + Num> Sub for Vector2<T> {
    type Output = Vector2<T>;

    fn sub(self, rhs: Self) -> Self::Output {
        Vector2 {
            x: self.x - rhs.x,
            y: self.y - rhs.y
        }
    }
}

impl<T: Copy + Default + Num + SubAssign> SubAssign for Vector2<T> {
    fn sub_assign(&mut self, rhs: Self) {
        self.x -= rhs.x;
        self.y -= rhs.y;
    }
}

impl<T: Copy + Default + Num> Mul for Vector2<T> {
    type Output = Vector2<T>;

    fn mul(self, rhs: Self) -> Self::Output {
        Vector2 {
            x: self.x * rhs.x,
            y: self.y * rhs.y
        }
    }
}

impl<T: Copy + Default + Num + MulAssign> MulAssign for Vector2<T> {
    fn mul_assign(&mut self, rhs: Self) {
        self.x *= rhs.x;
        self.y *= rhs.y;
    }
}

pub type Vector2f = Vector2<f32>;
pub type Vector2d = Vector2<f64>;
pub type Vector2i = Vector2<i32>;
pub type Vector2u = Vector2<u32>;
