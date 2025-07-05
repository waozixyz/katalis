use crate::types::*;
use raylib::math::Vector2;

pub struct World {
    pub tiles: Vec<Vec<Tile>>,
    pub trees: Vec<Tree>,  // NEW: Store trees separately
    pub width: usize,
    pub height: usize,
}

impl World {
    pub fn new(width: usize, height: usize) -> Self {
        let tiles = vec![vec![Tile {
            tile_type: TileType::Grass,
        }; height]; width];
        
        Self { 
            tiles, 
            trees: Vec::new(),  // NEW: Initialize empty tree vector
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
        
        // NEW: Generate trees after terrain
        self.generate_trees();
    }
    
    // NEW: Generate trees algorithmically
    fn generate_trees(&mut self) {
        use noise::{NoiseFn, Perlin};
        
        // Use a different seed for tree placement
        let tree_noise = Perlin::new(123);
        let tree_density_noise = Perlin::new(456);
        
        for x in 0..self.width {
            for y in 0..self.height {
                let tile = &self.tiles[x][y];
                
                // Only place trees on suitable terrain
                if !tile.tile_type.supports_trees() {
                    continue;
                }
                
                // Use noise to determine tree placement
                let tree_chance = tree_noise.get([x as f64 * 0.3, y as f64 * 0.3]);
                let density_modifier = tree_density_noise.get([x as f64 * 0.05, y as f64 * 0.05]);
                
                // Adjust tree density based on location (some areas denser than others)
                let final_chance = tree_chance + density_modifier * 0.3;
                
                // Tree placement threshold - adjust to control density
                if final_chance > 0.4 {
                    // Determine tree type based on location
                    let tree_type = if density_modifier > 0.2 {
                        TreeType::Pine    // Pine in "mountainous" areas
                    } else if final_chance > 0.7 {
                        TreeType::Oak     // Oak in dense forest areas
                    } else {
                        TreeType::Birch   // Birch elsewhere
                    };
                    
                    // Random position within the tile
                    let offset_x = (tree_noise.get([x as f64 * 1.7, y as f64 * 1.3]) * 0.6) as f32 * TILE_SIZE as f32;
                    let offset_y = (tree_noise.get([x as f64 * 1.3, y as f64 * 1.7]) * 0.6) as f32 * TILE_SIZE as f32;
                    
                    let tree_x = x as f32 * TILE_SIZE as f32 + TILE_SIZE as f32 * 0.5 + offset_x;
                    let tree_y = y as f32 * TILE_SIZE as f32 + TILE_SIZE as f32 * 0.5 + offset_y;
                    
                    // Random size within tree type's range
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
    
    // NEW: Get trees in a specific area (for rendering optimization)
    pub fn get_trees_in_bounds(&self, min_bound: Vector2, max_bound: Vector2) -> Vec<&Tree> {
        self.trees.iter()
            .filter(|tree| {
                tree.position.x >= min_bound.x && tree.position.x <= max_bound.x &&
                tree.position.y >= min_bound.y && tree.position.y <= max_bound.y
            })
            .collect()
    }
}