use raylib::prelude::*;
use super::{TileType, TILE_SIZE, STUMP_FADE_TIME};

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum TreeType {
    Oak,
    Pine,
    Birch,
}

impl TreeType {
    pub fn get_trunk_color(&self) -> Color {
        match self {
            TreeType::Oak => Color::new(101, 67, 33, 255),
            TreeType::Pine => Color::new(139, 69, 19, 255),
            TreeType::Birch => Color::new(245, 245, 220, 255),
        }
    }
    
    pub fn get_foliage_color(&self) -> Color {
        match self {
            TreeType::Oak => Color::new(34, 139, 34, 255),
            TreeType::Pine => Color::new(0, 100, 0, 255),
            TreeType::Birch => Color::new(50, 205, 50, 255),
        }
    }
    
    pub fn get_size_range(&self) -> (f32, f32) {
        match self {
            TreeType::Oak => (0.8, 1.3),
            TreeType::Pine => (1.0, 1.5),
            TreeType::Birch => (0.6, 1.0),
        }
    }
    
    pub fn can_grow_on(&self, tile_type: TileType) -> bool {
        match self {
            TreeType::Oak => matches!(tile_type, TileType::Grass),
            TreeType::Pine => matches!(tile_type, TileType::Grass | TileType::Stone),
            TreeType::Birch => matches!(tile_type, TileType::Grass),
        }
    }
    
    pub fn get_wood_yield(&self) -> u32 {
        match self {
            TreeType::Oak => 15,
            TreeType::Pine => 12,
            TreeType::Birch => 8,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum TreeState {
    Healthy,
    Stump(f32),   // Fade timer after being cut
}

#[derive(Clone, Debug)]
pub struct Tree {
    pub tree_type: TreeType,
    pub position: Vector2,
    pub size: f32,
    pub growth: f32,
    pub state: TreeState,
}

impl Tree {
    pub fn new(tree_type: TreeType, x: f32, y: f32, size: f32) -> Self {
        Self {
            tree_type,
            position: Vector2::new(x, y),
            size,
            growth: 1.0,
            state: TreeState::Healthy,
        }
    }
    
    pub fn update(&mut self, delta_time: f32) -> Option<u32> {
        match &mut self.state {
            TreeState::Stump(timer) => {
                *timer += delta_time;
                None
            }
            TreeState::Healthy => None,
        }
    }
    
    pub fn cut_down(&mut self) -> u32 {
        if matches!(self.state, TreeState::Healthy) {
            let wood_yield = self.tree_type.get_wood_yield();
            self.state = TreeState::Stump(0.0);
            println!("Tree cut down! Yielding {} wood", wood_yield); // Debug print
            wood_yield
        } else {
            println!("Tried to cut non-healthy tree"); // Debug print
            0
        }
    }
    
    pub fn should_remove(&self) -> bool {
        matches!(self.state, TreeState::Stump(timer) if timer >= STUMP_FADE_TIME)
    }
    
    pub fn is_healthy(&self) -> bool {
        matches!(self.state, TreeState::Healthy)
    }
    
    pub fn collides_with_point(&self, point: Vector2) -> bool {
        if !self.is_healthy() {
            return false;
        }
        
        let base_size = TILE_SIZE as f32 * 0.3 * self.size * self.growth;
        
        // Trunk collision (rectangle)
        let trunk_width = base_size * 0.4;
        let trunk_height = base_size * 1.5;
        let trunk_left = self.position.x - trunk_width * 0.5;
        let trunk_right = self.position.x + trunk_width * 0.5;
        let trunk_top = self.position.y - trunk_height * 0.5;
        let trunk_bottom = self.position.y + trunk_height * 0.5;
        
        let trunk_hit = point.x >= trunk_left && point.x <= trunk_right && 
                       point.y >= trunk_top && point.y <= trunk_bottom;
        
        // Foliage collision (circle)
        let foliage_radius = base_size * 0.8;
        let foliage_center = Vector2::new(self.position.x, self.position.y - trunk_height * 0.3);
        let foliage_distance = point.distance_to(foliage_center);
        let foliage_hit = foliage_distance <= foliage_radius;
        
        trunk_hit || foliage_hit
    }
    
    pub fn draw(&self, d: &mut RaylibMode2D<RaylibDrawHandle>) {
        let base_size = TILE_SIZE as f32 * 0.3 * self.size * self.growth;
        let trunk_height = base_size * 1.5;
        let trunk_width = base_size * 0.4;
        let foliage_radius = base_size * 0.8;
        
        match &self.state {
            TreeState::Healthy => {
                let trunk_color = self.tree_type.get_trunk_color();
                let foliage_color = self.tree_type.get_foliage_color();
                
                // Draw trunk
                d.draw_rectangle(
                    (self.position.x - trunk_width * 0.5) as i32,
                    (self.position.y - trunk_height * 0.5) as i32,
                    trunk_width as i32,
                    trunk_height as i32,
                    trunk_color
                );
                
                // Draw foliage
                let foliage_pos = Vector2::new(
                    self.position.x,
                    self.position.y - trunk_height * 0.3
                );
                
                d.draw_circle_v(foliage_pos, foliage_radius, foliage_color);
                d.draw_circle_lines_v(foliage_pos, foliage_radius, Color::new(0, 50, 0, 255));
            }
            TreeState::Stump(timer) => {
                let fade_progress = timer / STUMP_FADE_TIME;
                let alpha = (255.0 * (1.0 - fade_progress)) as u8;
                let stump_color = Color::new(101, 67, 33, alpha);
                
                // Draw small stump
                d.draw_rectangle(
                    (self.position.x - base_size * 0.15) as i32,
                    (self.position.y - base_size * 0.3) as i32,
                    (base_size * 0.3) as i32,
                    (base_size * 0.3) as i32,
                    stump_color
                );
            }
        }
    }
}