use raylib::prelude::*;
use std::collections::HashMap;

pub const TILE_SIZE: i32 = 32;
pub const PLAYER_SPEED: f32 = 200.0;
pub const CAMERA_SMOOTHNESS: f32 = 8.0;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum ResourceType {
    Wood,
    Stone,
    Food,
}

impl ResourceType {
    pub fn get_name(&self) -> &'static str {
        match self {
            ResourceType::Wood => "Wood",
            ResourceType::Stone => "Stone", 
            ResourceType::Food => "Food",
        }
    }
    
    pub fn get_color(&self) -> Color {
        match self {
            ResourceType::Wood => Color::BROWN,
            ResourceType::Stone => Color::GRAY,
            ResourceType::Food => Color::GREEN,
        }
    }
}

#[derive(Clone, Debug)]
pub struct Inventory {
    pub resources: HashMap<ResourceType, u32>,
}

impl Inventory {
    pub fn new() -> Self {
        let mut resources = HashMap::new();
        // Start with some basic resources for testing
        resources.insert(ResourceType::Wood, 50);
        resources.insert(ResourceType::Stone, 30);
        resources.insert(ResourceType::Food, 25);
        
        Self { resources }
    }
    
    pub fn get_amount(&self, resource: &ResourceType) -> u32 {
        *self.resources.get(resource).unwrap_or(&0)
    }
    
    pub fn add_resource(&mut self, resource: ResourceType, amount: u32) {
        *self.resources.entry(resource).or_insert(0) += amount;
    }
    
    pub fn remove_resource(&mut self, resource: ResourceType, amount: u32) -> bool {
        let current = self.get_amount(&resource);
        if current >= amount {
            *self.resources.entry(resource).or_insert(0) -= amount;
            true
        } else {
            false
        }
    }
}

// NEW: Tree types
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum TreeType {
    Oak,
    Pine,
    Birch,
}

impl TreeType {
    pub fn get_trunk_color(&self) -> Color {
        match self {
            TreeType::Oak => Color::new(101, 67, 33, 255),     // Dark brown
            TreeType::Pine => Color::new(139, 69, 19, 255),    // Saddle brown
            TreeType::Birch => Color::new(245, 245, 220, 255), // Beige
        }
    }
    
    pub fn get_foliage_color(&self) -> Color {
        match self {
            TreeType::Oak => Color::new(34, 139, 34, 255),     // Forest green
            TreeType::Pine => Color::new(0, 100, 0, 255),      // Dark green
            TreeType::Birch => Color::new(50, 205, 50, 255),   // Lime green
        }
    }
    
    pub fn get_size_range(&self) -> (f32, f32) {
        match self {
            TreeType::Oak => (0.8, 1.3),     // Medium to large
            TreeType::Pine => (1.0, 1.5),    // Large
            TreeType::Birch => (0.6, 1.0),   // Small to medium
        }
    }
    
    pub fn can_grow_on(&self, tile_type: TileType) -> bool {
        match self {
            TreeType::Oak => matches!(tile_type, TileType::Grass),
            TreeType::Pine => matches!(tile_type, TileType::Grass | TileType::Stone),
            TreeType::Birch => matches!(tile_type, TileType::Grass),
        }
    }
}

// NEW: Tree entity
#[derive(Clone, Copy, Debug)]
pub struct Tree {
    pub tree_type: TreeType,
    pub position: Vector2,
    pub size: f32,
    pub growth: f32, // 0.0 to 1.0, for future growth animation
}

impl Tree {
    pub fn new(tree_type: TreeType, x: f32, y: f32, size: f32) -> Self {
        Self {
            tree_type,
            position: Vector2::new(x, y),
            size,
            growth: 1.0, // Fully grown by default
        }
    }
    
    pub fn draw(&self, d: &mut RaylibMode2D<RaylibDrawHandle>) {
        let base_size = TILE_SIZE as f32 * 0.3 * self.size * self.growth;
        let trunk_height = base_size * 1.5;
        let foliage_radius = base_size * 0.8;
        
        // Draw trunk
        let trunk_color = self.tree_type.get_trunk_color();
        d.draw_rectangle(
            (self.position.x - base_size * 0.2) as i32,
            (self.position.y - trunk_height * 0.5) as i32,
            (base_size * 0.4) as i32,
            trunk_height as i32,
            trunk_color
        );
        
        // Draw foliage (circle for now, could be more complex)
        let foliage_color = self.tree_type.get_foliage_color();
        d.draw_circle_v(
            Vector2::new(self.position.x, self.position.y - trunk_height * 0.7),
            foliage_radius,
            foliage_color
        );
        
        // Add some detail - darker outline
        d.draw_circle_lines_v(
            Vector2::new(self.position.x, self.position.y - trunk_height * 0.7),
            foliage_radius,
            Color::new(0, 50, 0, 255)
        );
    }
}

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
            TileType::Grass => Color::new(85, 107, 47, 255),    // Darker olive green
            TileType::Stone => Color::new(105, 105, 105, 255),  // Dim gray
            TileType::Iron => Color::new(184, 134, 11, 255),    // Dark goldenrod
            TileType::Coal => Color::new(64, 64, 64, 255),      // Dark gray
            TileType::Water => Color::new(25, 25, 112, 255),    // Midnight blue
        }
    }
    
    pub fn get_overlay_color(&self) -> Option<Color> {
        match self {
            TileType::Iron => Some(Color::new(255, 165, 0, 80)), // Orange overlay
            TileType::Coal => Some(Color::new(0, 0, 0, 60)),     // Black overlay
            _ => None,
        }
    }
    
    // NEW: Check if this terrain supports trees
    pub fn supports_trees(&self) -> bool {
        matches!(self, TileType::Grass)
    }
}
