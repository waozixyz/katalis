use raylib::prelude::*;

#[derive(Clone, Copy, Debug)]
pub struct Tile {
    pub tile_type: TileType,
    pub resource_vein: Option<ResourceVein>,
}

#[derive(Clone, Copy, Debug, PartialEq, Hash, Eq)]
pub enum TileType {
    Grass,
    Stone,
    Water,
    Sand,    // NEW: Sandy areas near water
    Mountain, // NEW: Higher elevation stone areas
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct ResourceVein {
    pub vein_type: VeinType,
    pub richness: u32,  // How much resource is left in this tile
    pub max_richness: u32, // Starting amount
}

#[derive(Clone, Copy, Debug, PartialEq, Hash, Eq)]
pub enum VeinType {
    IronOre,
    CoalDeposit,
    StoneQuarry,
}

impl TileType {
    pub fn get_color(&self) -> Color {
        match self {
            TileType::Grass => Color::new(85, 107, 47, 255),
            TileType::Stone => Color::new(105, 105, 105, 255),
            TileType::Water => Color::new(25, 25, 112, 255),
            TileType::Sand => Color::new(194, 178, 128, 255),
            TileType::Mountain => Color::new(80, 80, 80, 255),
        }
    }
    
    pub fn supports_trees(&self) -> bool {
        matches!(self, TileType::Grass)
    }
    
    pub fn can_have_resource_vein(&self, vein_type: VeinType) -> bool {
        match vein_type {
            VeinType::IronOre => matches!(self, TileType::Stone | TileType::Mountain),
            VeinType::CoalDeposit => matches!(self, TileType::Grass | TileType::Stone),
            VeinType::StoneQuarry => matches!(self, TileType::Stone | TileType::Mountain),
        }
    }
}

impl VeinType {
    pub fn get_overlay_color(&self) -> Color {
        match self {
            VeinType::IronOre => Color::new(255, 165, 0, 120),      // Orange
            VeinType::CoalDeposit => Color::new(64, 64, 64, 150),   // Dark gray
            VeinType::StoneQuarry => Color::new(169, 169, 169, 100), // Light gray
        }
    }
    
    pub fn get_name(&self) -> &'static str {
        match self {
            VeinType::IronOre => "Iron Ore",
            VeinType::CoalDeposit => "Coal",
            VeinType::StoneQuarry => "Stone",
        }
    }
    
    pub fn get_mining_yield(&self) -> u32 {
        match self {
            VeinType::IronOre => 3,
            VeinType::CoalDeposit => 2,
            VeinType::StoneQuarry => 5,
        }
    }
    
    pub fn get_initial_richness_range(&self) -> (u32, u32) {
        match self {
            VeinType::IronOre => (50, 200),      // 50-200 iron ore per tile
            VeinType::CoalDeposit => (30, 150),  // 30-150 coal per tile
            VeinType::StoneQuarry => (100, 300), // 100-300 stone per tile
        }
    }
}

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