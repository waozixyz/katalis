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
            tiles: vec![vec![Tile::new(TileType::Grass); height]; width],
            trees: Vec::new(),
            game_time: GameTime::new(),
            width, 
            height 
        }
    }

    pub fn generate_simple(&mut self) {
        use noise::{NoiseFn, Perlin};
        
        // Multiple noise layers for different features
        let elevation_noise = Perlin::new(42);
        let moisture_noise = Perlin::new(123);
        let resource_noise = Perlin::new(456);
        
        // First pass: Generate base terrain
        for x in 0..self.width {
            for y in 0..self.height {
                let elevation = elevation_noise.get([x as f64 * 0.05, y as f64 * 0.05]);
                let moisture = moisture_noise.get([x as f64 * 0.08, y as f64 * 0.08]);
                let fine_detail = elevation_noise.get([x as f64 * 0.2, y as f64 * 0.2]);
                
                // Determine base terrain type
                let tile_type = match (elevation + fine_detail * 0.3, moisture) {
                    (e, _) if e > 0.4 => TileType::Mountain,    // High elevation = mountains
                    (e, _) if e > 0.2 => TileType::Stone,      // Medium elevation = stone
                    (_, m) if m < -0.4 => TileType::Water,     // Very wet = water
                    (e, m) if e < -0.2 && m < -0.1 => TileType::Sand, // Low + somewhat dry = sand
                    _ => TileType::Grass,                      // Default = grass
                };
                
                self.tiles[x][y] = Tile::new(tile_type);
            }
        }
        
        // Second pass: Generate resource veins
        self.generate_resource_veins();
        
        // Third pass: Generate trees
        self.generate_trees();
        
        self.print_generation_stats();
    }
    
    fn generate_resource_veins(&mut self) {
        use noise::{NoiseFn, Perlin};
        
        let iron_noise = Perlin::new(789);
        let coal_noise = Perlin::new(101112);
        let stone_noise = Perlin::new(131415);
        
        let mut iron_count = 0;
        let mut coal_count = 0;
        let mut stone_count = 0;
        
        for x in 0..self.width {
            for y in 0..self.height {
                let tile = &mut self.tiles[x][y];
                
                // Iron ore veins - prefer mountains and stone areas
                if tile.tile_type.can_have_resource_vein(VeinType::IronOre) {
                    let iron_value = iron_noise.get([x as f64 * 0.15, y as f64 * 0.15]);
                    let iron_density = iron_noise.get([x as f64 * 0.03, y as f64 * 0.03]);
                    
                    // Higher chance in mountains
                    let mountain_bonus = if matches!(tile.tile_type, TileType::Mountain) { 0.2 } else { 0.0 };
                    
                    if iron_value + iron_density * 0.4 + mountain_bonus > 0.6 {
                        let (min_rich, max_rich) = VeinType::IronOre.get_initial_richness_range();
                        let richness_factor = (iron_value + 1.0) * 0.5; // 0.0 to 1.0
                        let richness = min_rich + ((max_rich - min_rich) as f64 * richness_factor) as u32;
                        
                        tile.resource_vein = Some(ResourceVein::new(VeinType::IronOre, richness));
                        iron_count += 1;
                    }
                }
                
                // Coal deposits - prefer grass and some stone areas, form larger patches
                if tile.tile_type.can_have_resource_vein(VeinType::CoalDeposit) {
                    let coal_value = coal_noise.get([x as f64 * 0.12, y as f64 * 0.12]);
                    let coal_patch = coal_noise.get([x as f64 * 0.04, y as f64 * 0.04]);
                    
                    if coal_value + coal_patch * 0.6 > 0.5 {
                        let (min_rich, max_rich) = VeinType::CoalDeposit.get_initial_richness_range();
                        let richness_factor = (coal_value + 1.0) * 0.5;
                        let richness = min_rich + ((max_rich - min_rich) as f64 * richness_factor) as u32;
                        
                        tile.resource_vein = Some(ResourceVein::new(VeinType::CoalDeposit, richness));
                        coal_count += 1;
                    }
                }
                
                // Stone quarries - only in stone and mountain areas, less common
                if tile.tile_type.can_have_resource_vein(VeinType::StoneQuarry) && tile.resource_vein.is_none() {
                    let stone_value = stone_noise.get([x as f64 * 0.1, y as f64 * 0.1]);
                    
                    if stone_value > 0.7 {
                        let (min_rich, max_rich) = VeinType::StoneQuarry.get_initial_richness_range();
                        let richness_factor = (stone_value + 1.0) * 0.5;
                        let richness = min_rich + ((max_rich - min_rich) as f64 * richness_factor) as u32;
                        
                        tile.resource_vein = Some(ResourceVein::new(VeinType::StoneQuarry, richness));
                        stone_count += 1;
                    }
                }
            }
        }
        
        println!("Generated resource veins: {} iron, {} coal, {} stone", iron_count, coal_count, stone_count);
    }
    
    fn generate_trees(&mut self) {
        use noise::{NoiseFn, Perlin};
        
        let tree_noise = Perlin::new(123);
        let tree_density_noise = Perlin::new(456);
        
        for x in 0..self.width {
            for y in 0..self.height {
                let tile = &self.tiles[x][y];
                
                // Only grow trees on grass without resource veins
                if !tile.tile_type.supports_trees() || tile.resource_vein.is_some() {
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
    }
    
    fn print_generation_stats(&self) {
        let mut terrain_counts = std::collections::HashMap::new();
        let mut resource_counts = std::collections::HashMap::new();
        
        for row in &self.tiles {
            for tile in row {
                *terrain_counts.entry(tile.tile_type).or_insert(0) += 1;
                if let Some(vein) = &tile.resource_vein {
                    *resource_counts.entry(vein.vein_type).or_insert(0) += 1;
                }
            }
        }
        
        println!("World generation complete:");
        println!("  Trees: {}", self.trees.len());
        println!("  Terrain distribution: {:?}", terrain_counts);
        println!("  Resource distribution: {:?}", resource_counts);
    }
    
    pub fn update(&mut self, delta_time: f32, player: &mut Player) -> (u32, u32, u32, u32) { // Returns (wood, stone, iron, coal)
        let mut wood_gained = 0;
        let mut stone_gained = 0;
        let mut iron_gained = 0;
        let mut coal_gained = 0;
        
        // Update game time
        self.game_time.update(delta_time);
        
        // Check if player completed mining
        let mut mining_completed = false;
        let mut completed_mining = None;
        
        if let Some(mining) = &mut player.current_mining {
            if mining.update(delta_time) {
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
                MiningTarget::ResourceVein(vein_type, tile_x, tile_y) => {
                    if let Some(tile) = self.get_tile_mut(*tile_x, *tile_y) {
                        if let Some(vein) = &mut tile.resource_vein {
                            let yield_amount = vein.vein_type.get_mining_yield();
                            let mined = vein.mine(yield_amount);
                            
                            match vein_type {
                                VeinType::IronOre => iron_gained += mined,
                                VeinType::CoalDeposit => coal_gained += mined,
                                VeinType::StoneQuarry => stone_gained += mined,
                            }
                            
                            println!("Mined {} {} from vein! Remaining: {}", mined, vein.vein_type.get_name(), vein.richness);
                            
                            // Remove depleted veins
                            if vein.is_depleted() {
                                tile.resource_vein = None;
                                println!("Resource vein depleted!");
                            }
                        }
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
        self.trees.retain(|tree| !tree.should_remove());
        
        (wood_gained, stone_gained, iron_gained, coal_gained)
    }
    
    fn cut_tree_at_position(&mut self, target_pos: Vector2) -> u32 {
        for tree in &mut self.trees {
            let distance = tree.position.distance_to(target_pos);
            if tree.is_healthy() && distance < 32.0 {
                return tree.cut_down();
            }
        }
        0
    }
    
    pub fn try_start_mining(&mut self, player: &mut Player, target_pos: Vector2) {
        if player.is_mining() {
            return;
        }
        
        // First, try to find a resource vein to mine
        let tile_x = (target_pos.x / TILE_SIZE as f32).floor() as usize;
        let tile_y = (target_pos.y / TILE_SIZE as f32).floor() as usize;
        
        if let Some(tile) = self.get_tile(tile_x, tile_y) {
            if let Some(vein) = &tile.resource_vein {
                if ToolType::Pickaxe.can_mine_vein(vein.vein_type) && !vein.is_depleted() {
                    let tile_center = Vector2::new(
                        tile_x as f32 * TILE_SIZE as f32 + TILE_SIZE as f32 * 0.5,
                        tile_y as f32 * TILE_SIZE as f32 + TILE_SIZE as f32 * 0.5
                    );
                    
                    println!("Starting vein mining: {:?} at ({}, {})", vein.vein_type, tile_x, tile_y);
                    let mining_action = MiningAction::new(
                        ToolType::Pickaxe,
                        tile_center,
                        MiningTarget::ResourceVein(vein.vein_type, tile_x, tile_y)
                    );
                    player.start_mining(mining_action);
                    return;
                }
            }
        }
        
        // If no resource vein, try to find a tree to cut
        for tree in &self.trees {
            if tree.is_healthy() && tree.collides_with_point(target_pos) {
                println!("Starting tree mining at {:?}", tree.position);
                let mining_action = MiningAction::new(
                    ToolType::Axe,
                    tree.position,
                    MiningTarget::Tree(tree.position)
                );
                player.start_mining(mining_action);
                return;
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