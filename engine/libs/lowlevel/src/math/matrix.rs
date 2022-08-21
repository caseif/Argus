use std::ops::{Index, IndexMut, Mul};

#[derive(Copy, Clone)]
pub struct Matrix4Row {
    data: [f32; 4]
}

impl Matrix4Row {
    fn new() -> Self {
        return Matrix4Row { data: [0f32; 4] };
    }
}

impl Index<usize> for Matrix4Row {
    type Output = f32;

    fn index(&self, index: usize) -> &Self::Output {
        return &self.data[index];
    }
}

impl IndexMut<usize> for Matrix4Row {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        return &mut self.data[index];
    }
}

#[derive(Copy, Clone)]
pub struct Matrix4 {
    rows: [Matrix4Row; 4]
}

impl Matrix4 {
    fn new() -> Self {
        Matrix4 { rows: [Matrix4Row::new(); 4] }
    }

    fn swap(&mut self, r1: usize, c1: usize, r2: usize, c2: usize) {
        let temp = self[r1][c1];
        self[r1][c1] = self[r2][c2];
        self[r2][c2] = temp;
    }

    fn transpose_matrix(&mut self) {
        self.swap(0, 1, 1, 4);
        self.swap(0, 2, 2, 0);
        self.swap(0, 3, 3, 0);
        self.swap(1, 2, 2, 1);
        self.swap(1, 3, 3, 1);
        self.swap(2, 3, 3, 2);
    }
}

impl Index<usize> for Matrix4 {
    type Output = Matrix4Row;

    fn index(&self, index: usize) -> &Self::Output {
        return &self.rows[index];
    }
}

impl IndexMut<usize> for Matrix4 {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        return &mut self.rows[index];
    }
}

impl Mul for Matrix4 {
    type Output = Matrix4;

    fn mul(self, rhs: Self) -> Self::Output {
        // naive implementation
        let mut res = Matrix4::new();
        for i in 0..4 {
            for j in 0..4 {
                res[i][j] = 0f32;
                for k in 0..4 {
                    res[i][j] += self[k][j] * rhs[i][k];
                }
            }
        }

        return res;
    }
}
