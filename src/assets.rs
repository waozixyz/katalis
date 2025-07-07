use raylib::prelude::*;
use std::collections::HashMap;
use crate::types::{VeinType, TileType, ResourceType, BuildingType};
use crate::crafting::CraftableItem;

pub struct AssetManager {
    resource_textures: HashMap<VeinType, Texture2D>,
    terrain_textures: HashMap<TileType, Texture2D>,
    icon_textures: HashMap<ResourceType, Texture2D>,
    ui_textures: HashMap<String, Texture2D>, 
    building_textures: HashMap<CraftableItem, Texture2D>,
    crafting_icons: HashMap<CraftableItem, Texture2D>, 
}

impl AssetManager {
    pub fn new() -> Self {
        Self {
            resource_textures: HashMap::new(),
            terrain_textures: HashMap::new(),
            icon_textures: HashMap::new(),
            ui_textures: HashMap::new(),
            building_textures: HashMap::new(),
            crafting_icons: HashMap::new(),
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
            (TileType::Swamp, "swamp.png"),
        ];
        
        for (tile_type, filename) in terrain_types.iter() {
            let filepath = format!("assets/tiles/terrain/{}", filename);
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
            (VeinType::StoneQuarry, "stone_quarry.png"),
            (VeinType::CottonPatch, "cotton.png"),
        ];
        
        for (vein_type, filename) in resource_types.iter() {
            let filepath = format!("assets/tiles/resources/{}", filename);
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
        
        // Load icon textures for all available icons
        let icon_types = [
            (ResourceType::Wood, "wood.png"),
            (ResourceType::Stone, "stone.png"),
            (ResourceType::IronOre, "iron_ore.png"),
            (ResourceType::CopperOre, "copper_ore.png"),
            (ResourceType::Cotton, "cotton.png"),
            (ResourceType::Clay, "clay.png"),
            (ResourceType::Coal, "coal.png"),
            (ResourceType::Charcoal, "charcoal.png"),
            (ResourceType::IronBloom, "iron_bloom.png"),
            (ResourceType::WroughtIron, "wrought_iron.png"),
            (ResourceType::IronPlates, "iron_plates.png"),
            (ResourceType::IronGears, "iron_gears.png"),
            (ResourceType::MetalRods, "metal_rods.png"),
            (ResourceType::CharcoalPit, "charcoal_pit.png"),
            (ResourceType::BloomeryFurnace, "bloomery_furnace.png"),
            (ResourceType::StoneAnvil, "stone_anvil.png"),
            (ResourceType::SpinningWheel, "spinning wheel.png"),
            (ResourceType::WeavingMachine, "weaving_machine.png"),
        ];
        
        for (resource_type, filename) in icon_types.iter() {
            let filepath = format!("assets/icons/{}", filename);
            match rl.load_texture(thread, &filepath) {
                Ok(texture) => {
                    println!("Loaded icon texture: {}", filepath);
                    self.icon_textures.insert(*resource_type, texture);
                }
                Err(e) => {
                    println!("Warning: Could not load icon texture {}: {}", filepath, e);
                }
            }
        }

        let ui_assets = [
            ("inventory_slot", "assets/ui/inventory_slot.png"),
            ("crafting_slot", "assets/ui/crafting_slot.png"),
        ];
        
        for (name, filepath) in ui_assets.iter() {
            match rl.load_texture(thread, filepath) {
                Ok(texture) => {
                    println!("Loaded UI texture: {}", filepath);
                    self.ui_textures.insert(name.to_string(), texture);
                }
                Err(e) => {
                    println!("Warning: Could not load UI texture {}: {}", filepath, e);
                }
            }
        }


        let crafting_icon_types = [
            (CraftableItem::CharcoalPit, "charcoal_pit.png"),
            (CraftableItem::BloomeryFurnace, "bloomery_furnace.png"),
            (CraftableItem::StoneAnvil, "stone_anvil.png"),
            (CraftableItem::SpinningWheel, "spinning wheel.png"),
            (CraftableItem::WeavingMachine, "weaving_machine.png"),
        ];

        for (craftable_item, filename) in crafting_icon_types.iter() {
            let filepath = format!("assets/icons/{}", filename);
            match rl.load_texture(thread, &filepath) {
                Ok(texture) => {
                    println!("Loaded crafting icon: {}", filepath);
                    self.crafting_icons.insert(*craftable_item, texture);
                }
                Err(e) => {
                    println!("Warning: Could not load crafting icon {}: {}", filepath, e);
                }
            }
        }

        // Load building sprites
        let building_types = [
            (CraftableItem::CharcoalPit, "charcoal_pit.png"),
            (CraftableItem::BloomeryFurnace, "bloomery_furnace.png"),
            (CraftableItem::StoneAnvil, "stone_anvil.png"),
            (CraftableItem::SpinningWheel, "spinning_wheel.png"),
            (CraftableItem::WeavingMachine, "weaving_machine.png"),
            (CraftableItem::ConveyorBelt, "conveyor_belt.png"),
        ];

        for (building_item, filename) in building_types.iter() {
            let filepath = format!("assets/buildings/{}", filename);
            match rl.load_texture(thread, &filepath) {
                Ok(texture) => {
                    println!("Loaded building texture: {}", filepath);
                    self.building_textures.insert(*building_item, texture);
                }
                Err(e) => {
                    println!("Warning: Could not load building texture {}: {}", filepath, e);
                }
            }
        }
        
        Ok(())
    }

    pub fn get_ui_texture(&self, name: &str) -> Option<&Texture2D> {
        self.ui_textures.get(name)
    }
    
    pub fn has_ui_texture(&self, name: &str) -> bool {
        self.ui_textures.contains_key(name)
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
    
    pub fn has_terrain_texture(&self, tile_type: TileType) -> bool {
        self.terrain_textures.contains_key(&tile_type)
    }
    
    pub fn get_icon_texture(&self, resource_type: ResourceType) -> Option<&Texture2D> {
        self.icon_textures.get(&resource_type)
    }
    
    pub fn has_icon_texture(&self, resource_type: ResourceType) -> bool {
        self.icon_textures.contains_key(&resource_type)
    }

    pub fn get_crafting_icon(&self, item: CraftableItem) -> Option<&Texture2D> {
        self.crafting_icons.get(&item)
    }
    
    pub fn get_building_texture(&self, item: CraftableItem) -> Option<&Texture2D> {
        self.building_textures.get(&item)
    }
    
    pub fn get_building_texture_by_type(&self, building_type: BuildingType) -> Option<&Texture2D> {
        let craftable_item = building_type.to_craftable_item();
        self.building_textures.get(&craftable_item)
    }
    
    pub fn has_crafting_icon(&self, item: CraftableItem) -> bool {
        self.crafting_icons.contains_key(&item)
    }
}