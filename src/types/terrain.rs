use raylib::prelude::*;

#[derive(Clone, Copy, Debug)]
pub struct Tile {
    pub tile_type: TileType,
    pub resource_vein: Option<ResourceVein>,
    pub building: Option<BuildingType>,
    pub building_active: bool, // Whether the building is currently crafting/active
    pub is_building_origin: bool, // True only for the top-left tile of a building
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
    CopperOre,
    CottonPatch,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum BuildingType {
    CharcoalPit,
    BloomeryFurnace,
    StoneAnvil,
    SpinningWheel,
    WeavingMachine,
    ConveyorBelt,
}

impl BuildingType {
    pub fn from_resource_type(resource: &super::resources::ResourceType) -> Option<Self> {
        use super::resources::ResourceType;
        match resource {
            ResourceType::CharcoalPit => Some(BuildingType::CharcoalPit),
            ResourceType::BloomeryFurnace => Some(BuildingType::BloomeryFurnace),
            ResourceType::StoneAnvil => Some(BuildingType::StoneAnvil),
            ResourceType::SpinningWheel => Some(BuildingType::SpinningWheel),
            ResourceType::WeavingMachine => Some(BuildingType::WeavingMachine),
            ResourceType::ConveyorBelt => Some(BuildingType::ConveyorBelt),
            _ => None,
        }
    }
    
    pub fn get_size(&self) -> (i32, i32) {
        match self {
            BuildingType::CharcoalPit => (2, 2),
            BuildingType::BloomeryFurnace => (3, 3),
            BuildingType::StoneAnvil => (2, 2),
            BuildingType::SpinningWheel => (2, 2),
            BuildingType::WeavingMachine => (3, 2),
            BuildingType::ConveyorBelt => (1, 1),
        }
    }
    
    pub fn get_color(&self) -> Color {
        match self {
            BuildingType::CharcoalPit => Color::new(101, 67, 33, 255),
            BuildingType::BloomeryFurnace => Color::new(139, 69, 19, 255),
            BuildingType::StoneAnvil => Color::GRAY,
            BuildingType::SpinningWheel => Color::BROWN,
            BuildingType::WeavingMachine => Color::new(160, 82, 45, 255),
            BuildingType::ConveyorBelt => Color::DARKGRAY,
        }
    }
    
    pub fn to_craftable_item(&self) -> super::super::crafting::CraftableItem {
        use super::super::crafting::CraftableItem;
        match self {
            BuildingType::CharcoalPit => CraftableItem::CharcoalPit,
            BuildingType::BloomeryFurnace => CraftableItem::BloomeryFurnace,
            BuildingType::StoneAnvil => CraftableItem::StoneAnvil,
            BuildingType::SpinningWheel => CraftableItem::SpinningWheel,
            BuildingType::WeavingMachine => CraftableItem::WeavingMachine,
            BuildingType::ConveyorBelt => CraftableItem::ConveyorBelt,
        }
    }
    
    pub fn get_sprite_info(&self) -> (i32, i32, bool) {
        // Returns (frame_width_in_texture, frame_height_in_texture, is_animated)
        match self {
            BuildingType::CharcoalPit => (256, 256, true), // Each frame is 256x256 pixels, animated with 2 frames
            BuildingType::BloomeryFurnace => (256, 256, true), // Same dimensions as charcoal pit, animated with 2 frames
            BuildingType::StoneAnvil => (64, 64, false), // 2x2 tiles, static
            BuildingType::SpinningWheel => (64, 64, false), // 2x2 tiles, static
            BuildingType::WeavingMachine => (96, 64, false), // 3x2 tiles, static
            BuildingType::ConveyorBelt => (32, 32, false), // 1x1 tile, static
        }
    }
    
    pub fn get_name(&self) -> &'static str {
        match self {
            BuildingType::CharcoalPit => "Charcoal Pit",
            BuildingType::BloomeryFurnace => "Bloomery Furnace",
            BuildingType::StoneAnvil => "Stone Anvil",
            BuildingType::SpinningWheel => "Spinning Wheel",
            BuildingType::WeavingMachine => "Weaving Machine",
            BuildingType::ConveyorBelt => "Conveyor Belt",
        }
    }
}

impl VeinType {
    pub fn get_overlay_color(&self) -> Color {
        match self {
            VeinType::IronOre => Color::new(255, 165, 0, 120),
            VeinType::CoalDeposit => Color::new(64, 64, 64, 150),
            VeinType::StoneQuarry => Color::new(169, 169, 169, 100),
            VeinType::ClayDeposit => Color::new(139, 69, 19, 130),
            VeinType::CopperOre => Color::new(184, 115, 51, 120),
            VeinType::CottonPatch => Color::new(255, 255, 240, 100), // Off-white
        }
    }
    
    pub fn get_name(&self) -> &'static str {
        match self {
            VeinType::IronOre => "Iron Ore",
            VeinType::CoalDeposit => "Coal",
            VeinType::StoneQuarry => "Stone",
            VeinType::ClayDeposit => "Clay",
            VeinType::CopperOre => "Copper Ore",
            VeinType::CottonPatch => "Cotton",
        }
    }
    
    pub fn get_mining_yield(&self) -> u32 {
        match self {
            VeinType::IronOre => 3,
            VeinType::CoalDeposit => 2,
            VeinType::StoneQuarry => 5,
            VeinType::ClayDeposit => 4,
            VeinType::CopperOre => 4,
            VeinType::CottonPatch => 3,
        }
    }
    
    pub fn get_initial_richness_range(&self) -> (u32, u32) {
        match self {
            VeinType::IronOre => (50, 200),
            VeinType::CoalDeposit => (30, 150),
            VeinType::StoneQuarry => (100, 300),
            VeinType::ClayDeposit => (40, 180),
            VeinType::CopperOre => (60, 220),
            VeinType::CottonPatch => (15, 60),
        }
    }
    
    pub fn get_asset_filename(&self) -> &'static str {
        match self {
            VeinType::IronOre => "tiles/resources/iron.png",
            VeinType::CoalDeposit => "tiles/resources/coal.png",
            VeinType::StoneQuarry => "tiles/resources/stone_quarry.png",
            VeinType::ClayDeposit => "tiles/resources/clay.png",
            VeinType::CopperOre => "tiles/resources/copper.png",
            VeinType::CottonPatch => "tiles/resources/cotton.png",
        }
    }
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
            VeinType::CopperOre => matches!(self, TileType::Stone | TileType::Mountain | TileType::Grass),
            VeinType::CottonPatch => matches!(self, TileType::Grass), // Cotton only on grass
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
            building: None,
            building_active: false,
            is_building_origin: false,
        }
    }
    
    pub fn with_resource_vein(tile_type: TileType, vein: ResourceVein) -> Self {
        Self {
            tile_type,
            resource_vein: Some(vein),
            building: None,
            building_active: false,
            is_building_origin: false,
        }
    }
    
    pub fn can_be_mined(&self) -> bool {
        self.resource_vein.is_some() && !self.resource_vein.as_ref().unwrap().is_depleted()
    }
    
    pub fn can_place_building(&self) -> bool {
        self.building.is_none() && matches!(self.tile_type, TileType::Grass | TileType::Stone | TileType::Sand)
    }
}