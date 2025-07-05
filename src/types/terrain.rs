use raylib::prelude::*;

#[derive(Clone, Copy, Debug)]
pub struct Tile {
    pub tile_type: TileType,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum TileType {
    Grass,
    Stone,
    Iron,
    Coal,
    Water,
}

impl TileType {
    pub fn get_color(&self) -> Color {
        match self {
            TileType::Grass => Color::new(85, 107, 47, 255),
            TileType::Stone => Color::new(105, 105, 105, 255),
            TileType::Iron => Color::new(184, 134, 11, 255),
            TileType::Coal => Color::new(64, 64, 64, 255),
            TileType::Water => Color::new(25, 25, 112, 255),
        }
    }
    
    pub fn get_overlay_color(&self) -> Option<Color> {
        match self {
            TileType::Iron => Some(Color::new(255, 165, 0, 80)),
            TileType::Coal => Some(Color::new(0, 0, 0, 60)),
            _ => None,
        }
    }
    
    pub fn supports_trees(&self) -> bool {
        matches!(self, TileType::Grass)
    }
}
