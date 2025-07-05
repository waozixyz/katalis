use crate::types::*;
use raylib::math::Vector2;
use crate::Player;

pub struct World {
    pub tiles: Vec<Vec<Tile>>,
    pub trees: Vec<Tree>,
    pub game_time: GameTime,
    pub width: usize,
    pub height: usize,
}

impl World {
    pub fn new(width: usize, height: usize) -> Self {
        Self { 
            tiles: vec![vec![Tile { tile_type: TileType::Grass }; height]; width],
            trees: Vec::new(),
            game_time: GameTime::new(),
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
    
    pub fn update(&mut self, delta_time: f32, player: &mut Player) -> (u32, u32) {
        let mut wood_gained = 0;
        let mut stone_gained = 0;
        
        // Update game time
        self.game_time.update(delta_time);
        
        // Check if player completed mining - FIXED LOGIC
        let mut mining_completed = false;
        let mut completed_mining = None;
        
        if let Some(mining) = &mut player.current_mining {
            if mining.update(delta_time) {
                // Mining is complete
                completed_mining = Some(mining.clone());
                mining_completed = true;
            }
        }
        
        // Process completed mining
        if let Some(mining) = completed_mining {
            match &mining.target_type {
                MiningTarget::Tree(target_pos) => {
                    wood_gained += self.cut_tree_at_position(*target_pos);
                    println!("Tree mining completed! Gained {} wood", wood_gained);
                }
                MiningTarget::Tile(tile_type) => {
                    match tile_type {
                        TileType::Stone => {
                            stone_gained += 5;
                            println!("Stone mining completed! Gained {} stone", stone_gained);
                        }
                        TileType::Iron => {
                            stone_gained += 3;
                            println!("Iron mining completed! Gained {} iron", stone_gained);
                        }
                        TileType::Coal => {
                            stone_gained += 2;
                            println!("Coal mining completed! Gained {} coal", stone_gained);
                        }
                        _ => {}
                    }
                }
            }
        }
        
        // Clear mining if completed
        if mining_completed {
            player.current_mining = None;
        }
        
        // Update trees
        for tree in &mut self.trees {
            tree.update(delta_time);
        }
        
        // Remove fully faded stumps
        let trees_before = self.trees.len();
        self.trees.retain(|tree| !tree.should_remove());
        let trees_after = self.trees.len();
        
        if trees_before != trees_after {
            println!("Removed {} stumps", trees_before - trees_after);
        }
        
        (wood_gained, stone_gained)
    }
    
    fn cut_tree_at_position(&mut self, target_pos: Vector2) -> u32 {
        println!("Attempting to cut tree at position: {:?}", target_pos);
        for (i, tree) in self.trees.iter_mut().enumerate() {
            let distance = tree.position.distance_to(target_pos);
            println!("Tree {} at {:?}, distance: {}, healthy: {}", i, tree.position, distance, tree.is_healthy());
            
            if tree.is_healthy() && distance < 32.0 { // Increased tolerance
                let wood = tree.cut_down();
                println!("Successfully cut tree {}! Gained {} wood", i, wood);
                return wood;
            }
        }
        println!("No tree found to cut at position {:?}", target_pos);
        0
    }
    
    pub fn try_start_mining(&mut self, player: &mut Player, target_pos: Vector2) {
        // Don't start new mining if already mining
        if player.is_mining() {
            return;
        }
        
        // First, try to find a tree to cut
        for (i, tree) in self.trees.iter().enumerate() {
            if tree.is_healthy() && tree.collides_with_point(target_pos) {
                println!("Starting tree mining for tree {} at {:?}", i, tree.position);
                let mining_action = MiningAction::new(
                    ToolType::Axe,
                    tree.position,
                    MiningTarget::Tree(tree.position)
                );
                player.start_mining(mining_action);
                return;
            }
        }
        
        // If no tree found, try to mine terrain
        let tile_x = (target_pos.x / TILE_SIZE as f32).floor() as usize;
        let tile_y = (target_pos.y / TILE_SIZE as f32).floor() as usize;
        
        if let Some(tile) = self.get_tile(tile_x, tile_y) {
            if ToolType::Pickaxe.can_mine_tile(tile.tile_type) {
                let tile_center = Vector2::new(
                    tile_x as f32 * TILE_SIZE as f32 + TILE_SIZE as f32 * 0.5,
                    tile_y as f32 * TILE_SIZE as f32 + TILE_SIZE as f32 * 0.5
                );
                
                println!("Starting tile mining: {:?} at {:?}", tile.tile_type, tile_center);
                let mining_action = MiningAction::new(
                    ToolType::Pickaxe,
                    tile_center,
                    MiningTarget::Tile(tile.tile_type)
                );
                player.start_mining(mining_action);
            }
        }
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
}