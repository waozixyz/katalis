use crate::types::*;
use raylib::math::Vector2;

pub struct World {
    pub tiles: Vec<Vec<Tile>>,
    pub trees: Vec<Tree>,
    pub lasers: Vec<Laser>, // NEW: Store active lasers
    pub game_time: GameTime, // NEW: Game time system
    pub width: usize,
    pub height: usize,
}

impl World {
    pub fn new(width: usize, height: usize) -> Self {
        Self { 
            tiles: vec![vec![Tile { tile_type: TileType::Grass }; height]; width],
            trees: Vec::new(),
            lasers: Vec::new(), // NEW
            game_time: GameTime::new(), // NEW
            width, 
            height 
        }
    }
    
    pub fn generate_simple(&mut self) {
        use noise::{NoiseFn, Perlin};
        let perlin = Perlin::new(42);
        
        // Generate terrain
        for x in 0..self.width {
            for y in 0..self.height {
                let noise_val = perlin.get([x as f64 * 0.1, y as f64 * 0.1]);
                
                self.tiles[x][y].tile_type = match noise_val {
                    v if v > 0.3 => TileType::Stone,
                    v if v > 0.1 => TileType::Iron,
                    v if v < -0.3 => TileType::Water,
                    v if v < -0.1 => TileType::Coal,
                    _ => TileType::Grass,
                };
            }
        }
        
        self.generate_trees();
    }
    
    fn generate_trees(&mut self) {
        use noise::{NoiseFn, Perlin};
        
        let tree_noise = Perlin::new(123);
        let tree_density_noise = Perlin::new(456);
        
        for x in 0..self.width {
            for y in 0..self.height {
                let tile = &self.tiles[x][y];
                
                if !tile.tile_type.supports_trees() {
                    continue;
                }
                
                let tree_chance = tree_noise.get([x as f64 * 0.3, y as f64 * 0.3]);
                let density_modifier = tree_density_noise.get([x as f64 * 0.05, y as f64 * 0.05]);
                let final_chance = tree_chance + density_modifier * 0.3;
                
                if final_chance > 0.4 {
                    let tree_type = if density_modifier > 0.2 {
                        TreeType::Pine
                    } else if final_chance > 0.7 {
                        TreeType::Oak
                    } else {
                        TreeType::Birch
                    };
                    
                    let offset_x = (tree_noise.get([x as f64 * 1.7, y as f64 * 1.3]) * 0.6) as f32 * TILE_SIZE as f32;
                    let offset_y = (tree_noise.get([x as f64 * 1.3, y as f64 * 1.7]) * 0.6) as f32 * TILE_SIZE as f32;
                    
                    let tree_x = x as f32 * TILE_SIZE as f32 + TILE_SIZE as f32 * 0.5 + offset_x;
                    let tree_y = y as f32 * TILE_SIZE as f32 + TILE_SIZE as f32 * 0.5 + offset_y;
                    
                    let (min_size, max_size) = tree_type.get_size_range();
                    let size_noise = tree_noise.get([x as f64 * 2.1, y as f64 * 2.7]);
                    let size = min_size + (max_size - min_size) * ((size_noise + 1.0) * 0.5) as f32;
                    
                    let tree = Tree::new(tree_type, tree_x, tree_y, size);
                    self.trees.push(tree);
                }
            }
        }
        
        println!("Generated {} trees", self.trees.len());
    }
    
    // NEW: Update world systems
    pub fn update(&mut self, delta_time: f32) -> u32 {
        let mut wood_gained = 0;
        
        // Update game time
        self.game_time.update(delta_time);
        
        // Update lasers
        self.lasers.retain_mut(|laser| {
            laser.update(delta_time);
            laser.active
        });
        
        // Check laser-tree collisions
        for laser in &mut self.lasers {
            if !laser.active {
                continue;
            }
            
            for tree in &mut self.trees {
                if tree.collides_with_point(laser.current_pos) {
                    tree.ignite();
                    laser.active = false; // Laser stops when it hits something
                    break;
                }
            }
        }
        
        // Update trees and collect wood from burned trees
        for tree in &mut self.trees {
            if let Some(wood) = tree.update(delta_time) {
                wood_gained += wood;
            }
        }
        
        // Remove fully faded stumps
        self.trees.retain(|tree| !tree.should_remove());
        
        wood_gained
    }
    
    // NEW: Shoot laser
    pub fn shoot_laser(&mut self, start_pos: Vector2, direction: Vector2) {
        let laser = Laser::new(start_pos, direction);
        self.lasers.push(laser);
    }
    
    pub fn get_tile(&self, x: usize, y: usize) -> Option<&Tile> {
        if x < self.width && y < self.height {
            Some(&self.tiles[x][y])
        } else {
            None
        }
    }
    
    pub fn get_tile_mut(&mut self, x: usize, y: usize) -> Option<&mut Tile> {
        if x < self.width && y < self.height {
            Some(&mut self.tiles[x][y])
        } else {
            None
        }
    }
    
    pub fn get_trees_in_bounds(&self, min_bound: Vector2, max_bound: Vector2) -> Vec<&Tree> {
        self.trees.iter()
            .filter(|tree| {
                tree.position.x >= min_bound.x && tree.position.x <= max_bound.x &&
                tree.position.y >= min_bound.y && tree.position.y <= max_bound.y
            })
            .collect()
    }
    
    // NEW: Get lasers in bounds
    pub fn get_lasers_in_bounds(&self, min_bound: Vector2, max_bound: Vector2) -> Vec<&Laser> {
        self.lasers.iter()
            .filter(|laser| {
                laser.active &&
                laser.current_pos.x >= min_bound.x && laser.current_pos.x <= max_bound.x &&
                laser.current_pos.y >= min_bound.y && laser.current_pos.y <= max_bound.y
            })
            .collect()
    }
}