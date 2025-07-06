use raylib::prelude::*;

#[derive(Clone, Copy, Debug)]
pub struct Tile {
    pub tile_type: TileType,
    pub resource_vein: Option<ResourceVein>,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum TileType {
    Grass,
    Stone,
    Water,
    Sand,
    Mountain,
    Swamp,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ResourceVein {
    pub vein_type: VeinType,
    pub richness: u32,
    pub max_richness: u32,
}

#[derive(Clone, Copy, Debug, PartialEq, Hash, Eq)]
pub enum VeinType {
    IronOre,
    CoalDeposit,
    StoneQuarry,
    ClayDeposit,
    CopperOre,  // NEW
}

impl TileType {
    pub fn get_color(&self) -> Color {
        match self {
            TileType::Grass => Color::new(85, 107, 47, 255),
            TileType::Stone => Color::new(105, 105, 105, 255),
            TileType::Water => Color::new(25, 25, 112, 255),
            TileType::Sand => Color::new(194, 178, 128, 255),
            TileType::Mountain => Color::new(80, 80, 80, 255),
            TileType::Swamp => Color::new(61, 89, 41, 255),
        }
    }
    
    pub fn supports_trees(&self) -> bool {
        matches!(self, TileType::Grass | TileType::Swamp)
    }
    
    pub fn can_have_resource_vein(&self, vein_type: VeinType) -> bool {
        match vein_type {
            VeinType::IronOre => matches!(self, TileType::Stone | TileType::Mountain),
            VeinType::CoalDeposit => matches!(self, TileType::Grass | TileType::Stone),
            VeinType::StoneQuarry => matches!(self, TileType::Stone | TileType::Mountain),
            VeinType::ClayDeposit => matches!(self, TileType::Swamp | TileType::Sand),
            VeinType::CopperOre => matches!(self, TileType::Stone | TileType::Mountain | TileType::Grass), // Copper is more common
        }
    }
}

impl VeinType {
    pub fn get_overlay_color(&self) -> Color {
        match self {
            VeinType::IronOre => Color::new(255, 165, 0, 120),      // Orange
            VeinType::CoalDeposit => Color::new(64, 64, 64, 150),   // Dark gray
            VeinType::StoneQuarry => Color::new(169, 169, 169, 100), // Light gray
            VeinType::ClayDeposit => Color::new(139, 69, 19, 130),   // Brown/terracotta
            VeinType::CopperOre => Color::new(184, 115, 51, 120),    // Copper color
        }
    }
    
    pub fn get_name(&self) -> &'static str {
        match self {
            VeinType::IronOre => "Iron Ore",
            VeinType::CoalDeposit => "Coal",
            VeinType::StoneQuarry => "Stone",
            VeinType::ClayDeposit => "Clay",
            VeinType::CopperOre => "Copper Ore",
        }
    }
    
    pub fn get_mining_yield(&self) -> u32 {
        match self {
            VeinType::IronOre => 3,
            VeinType::CoalDeposit => 2,
            VeinType::StoneQuarry => 5,
            VeinType::ClayDeposit => 4,
            VeinType::CopperOre => 4,  // Good yield like clay
        }
    }
    
    pub fn get_initial_richness_range(&self) -> (u32, u32) {
        match self {
            VeinType::IronOre => (50, 200),
            VeinType::CoalDeposit => (30, 150),
            VeinType::StoneQuarry => (100, 300),
            VeinType::ClayDeposit => (40, 180),
            VeinType::CopperOre => (60, 220),  // Slightly more abundant than iron
        }
    }
    
    pub fn get_asset_filename(&self) -> &'static str {
        match self {
            VeinType::IronOre => "iron.png",
            VeinType::CoalDeposit => "coal.png",
            VeinType::StoneQuarry => "stone_quarry.png",
            VeinType::ClayDeposit => "clay.png",
            VeinType::CopperOre => "copper.png",
        }
    }
}

// Rest of the implementation remains the same...
impl ResourceVein {
    pub fn new(vein_type: VeinType, richness: u32) -> Self {
        Self {
            vein_type,
            richness,
            max_richness: richness,
        }
    }
    
    pub fn mine(&mut self, amount: u32) -> u32 {
        let mined = amount.min(self.richness);
        self.richness -= mined;
        mined
    }
    
    pub fn is_depleted(&self) -> bool {
        self.richness == 0
    }
    
    pub fn get_richness_percentage(&self) -> f32 {
        if self.max_richness == 0 {
            0.0
        } else {
            self.richness as f32 / self.max_richness as f32
        }
    }
}

impl Tile {
    pub fn new(tile_type: TileType) -> Self {
        Self {
            tile_type,
            resource_vein: None,
        }
    }
    
    pub fn with_resource_vein(tile_type: TileType, vein: ResourceVein) -> Self {
        Self {
            tile_type,
            resource_vein: Some(vein),
        }
    }
    
    pub fn can_be_mined(&self) -> bool {
        self.resource_vein.is_some() && !self.resource_vein.as_ref().unwrap().is_depleted()
    }
}