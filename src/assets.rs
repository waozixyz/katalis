use raylib::prelude::*;
use std::collections::HashMap;
use crate::types::{VeinType, TileType};

pub struct AssetManager {
    resource_textures: HashMap<VeinType, Texture2D>,
    terrain_textures: HashMap<TileType, Texture2D>, // NEW
}

impl AssetManager {
    pub fn new() -> Self {
        Self {
            resource_textures: HashMap::new(),
            terrain_textures: HashMap::new(), // NEW
        }
    }
    
    pub fn load_assets(&mut self, rl: &mut RaylibHandle, thread: &RaylibThread) -> Result<(), String> {
        // Load terrain textures (NEW)
        let terrain_types = [
            (TileType::Grass, "grass.png"),
            (TileType::Stone, "stone.png"),
            (TileType::Water, "water.png"),
            (TileType::Sand, "sand.png"),
            (TileType::Mountain, "mountain.png"),
            (TileType::Swamp, "swamp.png"), // Note: your file is named "swap.png" - you might need to rename it
        ];
        
        for (tile_type, filename) in terrain_types.iter() {
            let filepath = format!("assets/{}", filename);
            match rl.load_texture(thread, &filepath) {
                Ok(texture) => {
                    println!("Loaded terrain texture: {}", filepath);
                    self.terrain_textures.insert(*tile_type, texture);
                }
                Err(e) => {
                    println!("Warning: Could not load terrain texture {}: {}", filepath, e);
                }
            }
        }
        
        let resource_types = [
            (VeinType::IronOre, "iron.png"),
            (VeinType::CoalDeposit, "coal.png"),
            (VeinType::ClayDeposit, "clay.png"),
            (VeinType::CopperOre, "copper.png"),
            (VeinType::StoneQuarry, "stone_quarry.png"), // This is a resource overlay, not terrain
        ];
        
        for (vein_type, filename) in resource_types.iter() {
            let filepath = format!("assets/{}", filename);
            match rl.load_texture(thread, &filepath) {
                Ok(texture) => {
                    println!("Loaded resource texture: {}", filepath);
                    self.resource_textures.insert(*vein_type, texture);
                }
                Err(e) => {
                    println!("Warning: Could not load resource texture {}: {}", filepath, e);
                }
            }
        }
        
        Ok(())
    }
    
    pub fn get_resource_texture(&self, vein_type: VeinType) -> Option<&Texture2D> {
        self.resource_textures.get(&vein_type)
    }
    
    pub fn get_terrain_texture(&self, tile_type: TileType) -> Option<&Texture2D> { // NEW
        self.terrain_textures.get(&tile_type)
    }
    
    pub fn has_texture(&self, vein_type: VeinType) -> bool {
        self.resource_textures.contains_key(&vein_type)
    }
    
    pub fn has_terrain_texture(&self, tile_type: TileType) -> bool { // NEW
        self.terrain_textures.contains_key(&tile_type)
    }
}