use raylib::prelude::*;
use std::collections::HashMap;

pub const TILE_SIZE: i32 = 32;
pub const PLAYER_SPEED: f32 = 200.0;
pub const CAMERA_SMOOTHNESS: f32 = 8.0;
pub const LASER_SPEED: f32 = 800.0;
pub const LASER_MAX_DISTANCE: f32 = 400.0;
pub const TREE_BURN_TIME: f32 = 3.0; // 3 seconds to burn completely
pub const STUMP_FADE_TIME: f32 = 30.0; // 30 seconds for stump to fade (represents days)

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

// NEW: Game time system
#[derive(Clone, Debug)]
pub struct GameTime {
    pub total_seconds: f32,
    pub time_scale: f32, // How much faster game time moves than real time
}

impl GameTime {
    pub fn new() -> Self {
        Self {
            total_seconds: 43200.0, // Start at noon (12:00)
            time_scale: 60.0, // 1 real second = 1 game minute
        }
    }
    
    pub fn update(&mut self, delta_time: f32) {
        self.total_seconds += delta_time * self.time_scale;
        // Keep time within 24 hours (86400 seconds)
        self.total_seconds = self.total_seconds % 86400.0;
    }
    
    pub fn get_hours(&self) -> u32 {
        (self.total_seconds / 3600.0) as u32
    }
    
    pub fn get_minutes(&self) -> u32 {
        ((self.total_seconds % 3600.0) / 60.0) as u32
    }
    
    pub fn get_time_string(&self) -> String {
        format!("{:02}:{:02}", self.get_hours(), self.get_minutes())
    }
    
    pub fn is_day(&self) -> bool {
        let hour = self.get_hours();
        hour >= 6 && hour < 18 // Day from 6 AM to 6 PM
    }
    
    pub fn get_light_level(&self) -> f32 {
        let hour = self.get_hours() as f32 + (self.get_minutes() as f32 / 60.0);
        
        if hour >= 7.0 && hour <= 17.0 {
            1.0 // Full daylight
        } else if hour >= 19.0 || hour <= 5.0 {
            0.3 // Night
        } else {
            // Transition periods (dawn/dusk)
            if hour > 17.0 && hour < 19.0 {
                // Dusk: 17:00 to 19:00
                1.0 - (hour - 17.0) / 2.0 * 0.7
            } else {
                // Dawn: 5:00 to 7:00
                0.3 + (hour - 5.0) / 2.0 * 0.7
            }
        }
    }
}

// NEW: Laser projectile
#[derive(Clone, Debug)]
pub struct Laser {
    pub start_pos: Vector2,
    pub current_pos: Vector2,
    pub direction: Vector2,
    pub distance_traveled: f32,
    pub active: bool,
}

impl Laser {
    pub fn new(start_pos: Vector2, direction: Vector2) -> Self {
        Self {
            start_pos,
            current_pos: start_pos,
            direction: direction.normalized(),
            distance_traveled: 0.0,
            active: true,
        }
    }
    
    pub fn update(&mut self, delta_time: f32) {
        if !self.active {
            return;
        }
        
        let movement = self.direction * LASER_SPEED * delta_time;
        self.current_pos = self.current_pos + movement;
        self.distance_traveled += movement.length();
        
        if self.distance_traveled >= LASER_MAX_DISTANCE {
            self.active = false;
        }
    }
    
    pub fn draw(&self, d: &mut RaylibMode2D<RaylibDrawHandle>) {
        if !self.active {
            return;
        }
        
        // Draw laser beam
        d.draw_line_v(self.start_pos, self.current_pos, Color::RED);
        d.draw_circle_v(self.current_pos, 3.0, Color::new(255, 100, 100, 255));
    }
}

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
    
    // NEW: Wood yield when burned
    pub fn get_wood_yield(&self) -> u32 {
        match self {
            TreeType::Oak => 15,
            TreeType::Pine => 12,
            TreeType::Birch => 8,
        }
    }
}

// NEW: Tree states
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum TreeState {
    Healthy,
    Burning(f32), // Burn timer
    Stump(f32),   // Fade timer
}

#[derive(Clone, Debug)]
pub struct Tree {
    pub tree_type: TreeType,
    pub position: Vector2,
    pub size: f32,
    pub growth: f32,
    pub state: TreeState, // NEW: Track tree state
}

impl Tree {
    pub fn new(tree_type: TreeType, x: f32, y: f32, size: f32) -> Self {
        Self {
            tree_type,
            position: Vector2::new(x, y),
            size,
            growth: 1.0,
            state: TreeState::Healthy, // NEW: Start healthy
        }
    }
    
    // NEW: Update tree state
    pub fn update(&mut self, delta_time: f32) -> Option<u32> {
        match &mut self.state {
            TreeState::Burning(timer) => {
                *timer += delta_time;
                if *timer >= TREE_BURN_TIME {
                    let wood_yield = self.tree_type.get_wood_yield();
                    self.state = TreeState::Stump(0.0);
                    Some(wood_yield)
                } else {
                    None
                }
            }
            TreeState::Stump(timer) => {
                *timer += delta_time;
                if *timer >= STUMP_FADE_TIME {
                    // Tree should be removed from world
                }
                None
            }
            TreeState::Healthy => None,
        }
    }
    
    // NEW: Start burning
    pub fn ignite(&mut self) {
        if matches!(self.state, TreeState::Healthy) {
            self.state = TreeState::Burning(0.0);
        }
    }
    
    // NEW: Check if tree should be removed
    pub fn should_remove(&self) -> bool {
        matches!(self.state, TreeState::Stump(timer) if timer >= STUMP_FADE_TIME)
    }
    
    // NEW: Check collision with point
    pub fn collides_with_point(&self, point: Vector2) -> bool {
        let base_size = TILE_SIZE as f32 * 0.3 * self.size * self.growth;
        let foliage_radius = base_size * 0.8;
        
        // Check collision with foliage area
        let foliage_pos = Vector2::new(
            self.position.x,
            self.position.y - base_size * 1.5 * 0.7
        );
        
        point.distance_to(foliage_pos) <= foliage_radius
    }
    pub fn draw(&self, d: &mut RaylibMode2D<RaylibDrawHandle>) {
        let base_size = TILE_SIZE as f32 * 0.3 * self.size * self.growth;
        let trunk_height = base_size * 1.5;
        let foliage_radius = base_size * 0.8;
        
        // Rotation compensation for camera angle
        let rotation_compensation = 20.0; // Opposite of camera rotation
        let angle_rad = rotation_compensation * std::f32::consts::PI / 180.0;
        
        match &self.state {
            TreeState::Healthy => {
                // Draw normal tree
                let trunk_color = self.tree_type.get_trunk_color();
                let foliage_color = self.tree_type.get_foliage_color();
                
                // Calculate rotated trunk position
                let trunk_offset_y = -trunk_height * 0.5;
                let rotated_trunk_offset = Vector2::new(
                    0.0 * angle_rad.cos() - trunk_offset_y * angle_rad.sin(),
                    0.0 * angle_rad.sin() + trunk_offset_y * angle_rad.cos()
                );
                
                // Draw trunk (rotated)
                d.draw_rectangle(
                    (self.position.x + rotated_trunk_offset.x - base_size * 0.2) as i32,
                    (self.position.y + rotated_trunk_offset.y) as i32,
                    (base_size * 0.4) as i32,
                    trunk_height as i32,
                    trunk_color
                );
                
                // Calculate rotated foliage position
                let foliage_offset_y = -trunk_height * 0.7;
                let rotated_foliage_offset = Vector2::new(
                    0.0 * angle_rad.cos() - foliage_offset_y * angle_rad.sin(),
                    0.0 * angle_rad.sin() + foliage_offset_y * angle_rad.cos()
                );
                
                let foliage_pos = Vector2::new(
                    self.position.x + rotated_foliage_offset.x,
                    self.position.y + rotated_foliage_offset.y
                );
                
                // Draw foliage
                d.draw_circle_v(foliage_pos, foliage_radius, foliage_color);
                d.draw_circle_lines_v(foliage_pos, foliage_radius, Color::new(0, 50, 0, 255));
            }
            TreeState::Burning(timer) => {
                // Draw burning tree with fire effects
                let burn_progress = timer / TREE_BURN_TIME;
                let trunk_color = Color::new(50, 25, 0, 255);
                let fire_color = if (timer * 10.0) as i32 % 2 == 0 {
                    Color::new(255, 100, 0, 255)
                } else {
                    Color::new(255, 0, 0, 255)
                };
                
                // Calculate rotated positions
                let trunk_offset_y = -trunk_height * 0.5;
                let rotated_trunk_offset = Vector2::new(
                    0.0 * angle_rad.cos() - trunk_offset_y * angle_rad.sin(),
                    0.0 * angle_rad.sin() + trunk_offset_y * angle_rad.cos()
                );
                
                let foliage_offset_y = -trunk_height * 0.7;
                let rotated_foliage_offset = Vector2::new(
                    0.0 * angle_rad.cos() - foliage_offset_y * angle_rad.sin(),
                    0.0 * angle_rad.sin() + foliage_offset_y * angle_rad.cos()
                );
                
                // Draw trunk (getting darker)
                d.draw_rectangle(
                    (self.position.x + rotated_trunk_offset.x - base_size * 0.2) as i32,
                    (self.position.y + rotated_trunk_offset.y) as i32,
                    (base_size * 0.4) as i32,
                    trunk_height as i32,
                    trunk_color
                );
                
                // Draw burning foliage (shrinking as it burns)
                let burning_radius = foliage_radius * (1.0 - burn_progress);
                if burning_radius > 0.0 {
                    let foliage_pos = Vector2::new(
                        self.position.x + rotated_foliage_offset.x,
                        self.position.y + rotated_foliage_offset.y
                    );
                    d.draw_circle_v(foliage_pos, burning_radius, fire_color);
                }
                
                // Draw fire particles (also rotated)
                for i in 0..3 {
                    let particle_offset_y = foliage_offset_y - 10.0 - (timer * 3.0 + i as f32).cos() * 8.0;
                    let particle_offset_x = (timer * 5.0 + i as f32).sin() * 5.0;
                    
                    let rotated_particle_offset = Vector2::new(
                        particle_offset_x * angle_rad.cos() - particle_offset_y * angle_rad.sin(),
                        particle_offset_x * angle_rad.sin() + particle_offset_y * angle_rad.cos()
                    );
                    
                    d.draw_circle_v(
                        Vector2::new(
                            self.position.x + rotated_particle_offset.x,
                            self.position.y + rotated_particle_offset.y
                        ),
                        2.0,
                        Color::new(255, 150, 0, 150)
                    );
                }
            }
            TreeState::Stump(timer) => {
                // Draw black stump that fades over time (no rotation needed for stump)
                let fade_progress = timer / STUMP_FADE_TIME;
                let alpha = (255.0 * (1.0 - fade_progress)) as u8;
                let stump_color = Color::new(20, 20, 20, alpha);
                
                // Draw small stump (stumps stay at ground level)
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