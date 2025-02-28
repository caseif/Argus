
#[derive(Clone, Debug)]
pub struct TextureData {
    width: u32,
    height: u32,
    bpp: u32,
    data: Vec<u8>,
}

impl TextureData {
    #[must_use]
    pub fn new(width: u32, height: u32, bpp: u32, data: Vec<u8>) -> TextureData {
        if data.len() != (width * height * bpp) as usize {
            panic!("Texture dimensions do not match pixel data length");
        }
        Self { width, height, bpp, data }
    }
    
    #[must_use]
    pub fn get_width(&self) -> u32 {
        self.width
    }
    
    #[must_use]
    pub fn get_height(&self) -> u32 {
        self.height
    }
    
    #[must_use]
    pub fn get_bpp(&self) -> u32 {
        self.bpp
    }
    
    #[must_use]
    pub fn get_pixel_data(&self) -> &Vec<u8> {
        &self.data
    }
}
