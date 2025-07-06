use raylib::prelude::*;
use std::collections::HashMap;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum ResourceType {
    // Basic resources
    Wood,
    Stone,
    IronOre,
    Coal,
    Clay,
    CopperOre,
    Cotton, // NEW
    
    // Processed materials
    Charcoal, // NEW
    IronBloom, // NEW
    WroughtIron, // NEW
    IronPlates, // NEW
    IronGears, // NEW
    MetalRods, // NEW
    Threads, // NEW
    Fabric, // NEW
    ClothStrips, // NEW
}

impl ResourceType {
    pub fn get_name(&self) -> &'static str {
        match self {
            ResourceType::Wood => "Wood",
            ResourceType::Stone => "Stone", 
            ResourceType::IronOre => "Iron Ore",
            ResourceType::Coal => "Coal",
            ResourceType::Clay => "Clay",
            ResourceType::CopperOre => "Copper Ore",
            ResourceType::Cotton => "Cotton",
            ResourceType::Charcoal => "Charcoal",
            ResourceType::IronBloom => "Iron Bloom",
            ResourceType::WroughtIron => "Wrought Iron",
            ResourceType::IronPlates => "Iron Plates",
            ResourceType::IronGears => "Iron Gears",
            ResourceType::MetalRods => "Metal Rods",
            ResourceType::Threads => "Threads",
            ResourceType::Fabric => "Fabric",
            ResourceType::ClothStrips => "Cloth Strips",
        }
    }
    
    pub fn get_color(&self) -> Color {
        match self {
            ResourceType::Wood => Color::BROWN,
            ResourceType::Stone => Color::GRAY,
            ResourceType::IronOre => Color::new(255, 165, 0, 255),
            ResourceType::Coal => Color::new(64, 64, 64, 255),
            ResourceType::Clay => Color::new(139, 69, 19, 255),
            ResourceType::CopperOre => Color::new(184, 115, 51, 255),
            ResourceType::Cotton => Color::new(255, 255, 240, 255), // Off-white
            ResourceType::Charcoal => Color::new(36, 36, 36, 255), // Dark gray
            ResourceType::IronBloom => Color::new(160, 160, 160, 255), // Light gray
            ResourceType::WroughtIron => Color::new(128, 128, 128, 255), // Medium gray
            ResourceType::IronPlates => Color::new(192, 192, 192, 255), // Silver
            ResourceType::IronGears => Color::new(169, 169, 169, 255), // Dark gray
            ResourceType::MetalRods => Color::new(105, 105, 105, 255), // Dim gray
            ResourceType::Threads => Color::new(245, 245, 220, 255), // Beige
            ResourceType::Fabric => Color::new(230, 230, 250, 255), // Lavender
            ResourceType::ClothStrips => Color::new(255, 228, 196, 255), // Bisque
        }
    }
    
    pub fn get_icon_filename(&self) -> &'static str {
        match self {
            ResourceType::Wood => "icons/wood.png",
            ResourceType::Stone => "icons/stone.png",
            ResourceType::IronOre => "icons/iron_ore.png",
            ResourceType::Coal => "icons/coal.png",
            ResourceType::Clay => "icons/clay.png",
            ResourceType::CopperOre => "icons/copper_ore.png",
            ResourceType::Cotton => "icons/cotton.png",
            ResourceType::Charcoal => "icons/charcoal.png",
            ResourceType::IronBloom => "icons/iron_bloom.png",
            ResourceType::WroughtIron => "icons/wrought_iron.png",
            ResourceType::IronPlates => "icons/iron_plates.png",
            ResourceType::IronGears => "icons/iron_gears.png",
            ResourceType::MetalRods => "icons/metal_rods.png",
            ResourceType::Threads => "icons/threads.png",
            ResourceType::Fabric => "icons/fabric.png",
            ResourceType::ClothStrips => "icons/cloth_strips.png",
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
        
        // Basic starting resources
        resources.insert(ResourceType::Wood, 50);
        resources.insert(ResourceType::Stone, 30);
        resources.insert(ResourceType::IronOre, 0);
        resources.insert(ResourceType::Coal, 0);
        resources.insert(ResourceType::Clay, 0);
        resources.insert(ResourceType::CopperOre, 0);
        resources.insert(ResourceType::Cotton, 0);
        
        // Processed materials start at 0
        resources.insert(ResourceType::Charcoal, 0);
        resources.insert(ResourceType::IronBloom, 0);
        resources.insert(ResourceType::WroughtIron, 0);
        resources.insert(ResourceType::IronPlates, 0);
        resources.insert(ResourceType::IronGears, 0);
        resources.insert(ResourceType::MetalRods, 0);
        resources.insert(ResourceType::Threads, 0);
        resources.insert(ResourceType::Fabric, 0);
        resources.insert(ResourceType::ClothStrips, 0);
        
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