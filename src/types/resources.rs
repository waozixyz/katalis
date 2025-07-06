use raylib::prelude::*;
use std::collections::HashMap;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum ResourceType {
    Wood,
    Stone,
    IronOre,
    Coal,
    Clay,
    CopperOre,  // NEW
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
        }
    }
    
    pub fn get_color(&self) -> Color {
        match self {
            ResourceType::Wood => Color::BROWN,
            ResourceType::Stone => Color::GRAY,
            ResourceType::IronOre => Color::new(255, 165, 0, 255),
            ResourceType::Coal => Color::new(64, 64, 64, 255),
            ResourceType::Clay => Color::new(139, 69, 19, 255),
            ResourceType::CopperOre => Color::new(184, 115, 51, 255), // Copper color
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
        resources.insert(ResourceType::IronOre, 0);
        resources.insert(ResourceType::Coal, 0);
        resources.insert(ResourceType::Clay, 0);
        resources.insert(ResourceType::CopperOre, 0);  // NEW
        
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