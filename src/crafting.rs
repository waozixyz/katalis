use crate::types::*;
use std::collections::HashMap;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum CraftingCategory {
    BasicMaterials,
    Metallurgy,
    Textiles,
    Structures,
    Automation,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum CraftableItem {
    // Intermediate materials
    Charcoal,
    IronBloom,
    WroughtIron,
    IronPlates,
    IronGears,
    MetalRods,
    Threads,
    Fabric,
    ClothStrips,
    
    // Buildings/Structures
    CharcoalPit,
    BloomeryFurnace,
    StoneAnvil,
    SpinningWheel,
    WeavingMachine,
    ConveyorBelt,
}

#[derive(Clone, Debug)]
pub struct CraftingRecipe {
    pub inputs: Vec<(ResourceType, u32)>,
    pub output: (CraftableItem, u32),
    pub crafting_time: f32, // In seconds
    pub requires_structure: Option<StructureType>, // What structure is needed to craft this
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum StructureType {
    CharcoalPit,
    BloomeryFurnace,
    StoneAnvil,
    SpinningWheel,
    WeavingMachine,
    Manual, // Can be crafted by hand
}

impl CraftingCategory {
    pub fn get_name(&self) -> &'static str {
        match self {
            CraftingCategory::BasicMaterials => "Basic",
            CraftingCategory::Metallurgy => "Metal",
            CraftingCategory::Textiles => "Textiles",
            CraftingCategory::Structures => "Buildings",
            CraftingCategory::Automation => "Automation",
        }
    }
    
    pub fn get_items(&self) -> Vec<CraftableItem> {
        match self {
            CraftingCategory::BasicMaterials => vec![CraftableItem::Charcoal],
            CraftingCategory::Metallurgy => vec![
                CraftableItem::IronBloom,
                CraftableItem::WroughtIron,
                CraftableItem::IronPlates,
                CraftableItem::IronGears,
                CraftableItem::MetalRods,
            ],
            CraftingCategory::Textiles => vec![
                CraftableItem::Threads,
                CraftableItem::Fabric,
                CraftableItem::ClothStrips,
            ],
            CraftingCategory::Structures => vec![
                CraftableItem::CharcoalPit,
                CraftableItem::BloomeryFurnace,
                CraftableItem::StoneAnvil,
                CraftableItem::SpinningWheel,
                CraftableItem::WeavingMachine,
            ],
            CraftingCategory::Automation => vec![
                CraftableItem::ConveyorBelt,
            ],
        }
    }
}

impl CraftableItem {
    pub fn get_category(&self) -> CraftingCategory {
        match self {
            CraftableItem::Charcoal => CraftingCategory::BasicMaterials,
            CraftableItem::IronBloom | CraftableItem::WroughtIron | CraftableItem::IronPlates | 
            CraftableItem::IronGears | CraftableItem::MetalRods => CraftingCategory::Metallurgy,
            CraftableItem::Threads | CraftableItem::Fabric | CraftableItem::ClothStrips => CraftingCategory::Textiles,
            CraftableItem::CharcoalPit | CraftableItem::BloomeryFurnace | CraftableItem::StoneAnvil |
            CraftableItem::SpinningWheel | CraftableItem::WeavingMachine => CraftingCategory::Structures,
            CraftableItem::ConveyorBelt => CraftingCategory::Automation,
        }
    }

    pub fn get_name(&self) -> &'static str {
        match self {
            CraftableItem::Charcoal => "Charcoal",
            CraftableItem::IronBloom => "Iron Bloom",
            CraftableItem::WroughtIron => "Wrought Iron",
            CraftableItem::IronPlates => "Iron Plates",
            CraftableItem::IronGears => "Iron Gears",
            CraftableItem::MetalRods => "Metal Rods",
            CraftableItem::Threads => "Threads",
            CraftableItem::Fabric => "Fabric",
            CraftableItem::ClothStrips => "Cloth Strips",
            CraftableItem::CharcoalPit => "Charcoal Pit",
            CraftableItem::BloomeryFurnace => "Bloomery Furnace",
            CraftableItem::StoneAnvil => "Stone Anvil",
            CraftableItem::SpinningWheel => "Spinning Wheel",
            CraftableItem::WeavingMachine => "Weaving Machine",
            CraftableItem::ConveyorBelt => "Conveyor Belt",
        }
    }
    
    pub fn get_icon_filename(&self) -> &'static str {
        match self {
            CraftableItem::Charcoal => "icons/charcoal.png",
            CraftableItem::IronBloom => "icons/iron_bloom.png",
            CraftableItem::WroughtIron => "icons/wrought_iron.png",
            CraftableItem::IronPlates => "icons/iron_plates.png",
            CraftableItem::IronGears => "icons/iron_gears.png",
            CraftableItem::MetalRods => "icons/metal_rods.png",
            CraftableItem::Threads => "icons/threads.png",
            CraftableItem::Fabric => "icons/fabric.png",
            CraftableItem::ClothStrips => "icons/cloth_strips.png",
            CraftableItem::CharcoalPit => "icons/charcoal_pit.png",
            CraftableItem::BloomeryFurnace => "icons/bloomery_furnace.png",
            CraftableItem::StoneAnvil => "icons/stone_anvil.png",
            CraftableItem::SpinningWheel => "icons/spinning_wheel.png",
            CraftableItem::WeavingMachine => "icons/weaving_machine.png",
            CraftableItem::ConveyorBelt => "icons/conveyor_belt.png",
        }
    }
}

pub struct CraftingSystem {
    pub recipes: HashMap<CraftableItem, CraftingRecipe>,
}

impl CraftingSystem {
    pub fn new() -> Self {
        let mut recipes = HashMap::new();
        
        // 1. Charcoal Pit (structure)
        recipes.insert(CraftableItem::CharcoalPit, CraftingRecipe {
            inputs: vec![(ResourceType::Stone, 10), (ResourceType::Clay, 5)],
            output: (CraftableItem::CharcoalPit, 1),
            crafting_time: 5.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // 2. Bloomery Furnace (structure)
        recipes.insert(CraftableItem::BloomeryFurnace, CraftingRecipe {
            inputs: vec![(ResourceType::Stone, 15), (ResourceType::Clay, 8)],
            output: (CraftableItem::BloomeryFurnace, 1),
            crafting_time: 8.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // 3. Stone Anvil (structure)
        recipes.insert(CraftableItem::StoneAnvil, CraftingRecipe {
            inputs: vec![(ResourceType::Stone, 20)],
            output: (CraftableItem::StoneAnvil, 1),
            crafting_time: 6.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // 4. Spinning Wheel (structure)
        recipes.insert(CraftableItem::SpinningWheel, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 25)],
            output: (CraftableItem::SpinningWheel, 1),
            crafting_time: 10.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // 5. Weaving Machine (structure)
        recipes.insert(CraftableItem::WeavingMachine, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 35)],
            output: (CraftableItem::WeavingMachine, 1),
            crafting_time: 15.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Material processing recipes
        
        // Charcoal (requires Charcoal Pit + Wood)
        recipes.insert(CraftableItem::Charcoal, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 3)],
            output: (CraftableItem::Charcoal, 2),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::CharcoalPit),
        });
        
        // Iron Bloom (requires Bloomery Furnace + Charcoal + Iron Ore)
        recipes.insert(CraftableItem::IronBloom, CraftingRecipe {
            inputs: vec![(ResourceType::IronOre, 2), (ResourceType::Coal, 1)], // Using charcoal when we have it
            output: (CraftableItem::IronBloom, 1),
            crafting_time: 6.0,
            requires_structure: Some(StructureType::BloomeryFurnace),
        });
        
        // Wrought Iron (requires Stone Anvil + Iron Bloom)
        recipes.insert(CraftableItem::WroughtIron, CraftingRecipe {
            inputs: vec![(ResourceType::IronBloom, 1)],
            output: (CraftableItem::WroughtIron, 1),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::StoneAnvil),
        });
        
        // Iron Plates (requires Stone Anvil + Wrought Iron)
        recipes.insert(CraftableItem::IronPlates, CraftingRecipe {
            inputs: vec![(ResourceType::WroughtIron, 1)],
            output: (CraftableItem::IronPlates, 2),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::StoneAnvil),
        });
        
        // Iron Gears (manual crafting from Iron Plates)
        recipes.insert(CraftableItem::IronGears, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 2)],
            output: (CraftableItem::IronGears, 1),
            crafting_time: 2.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Metal Rods (manual crafting from Iron Plates)
        recipes.insert(CraftableItem::MetalRods, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 1)],
            output: (CraftableItem::MetalRods, 2),
            crafting_time: 1.5,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Threads (requires Spinning Wheel + Cotton)
        recipes.insert(CraftableItem::Threads, CraftingRecipe {
            inputs: vec![(ResourceType::Cotton, 2)],
            output: (CraftableItem::Threads, 3),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::SpinningWheel),
        });
        
        // Fabric (requires Weaving Machine + Threads)
        recipes.insert(CraftableItem::Fabric, CraftingRecipe {
            inputs: vec![(ResourceType::Threads, 3)],
            output: (CraftableItem::Fabric, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::WeavingMachine),
        });
        
        // Cloth Strips (manual cutting from Fabric)
        recipes.insert(CraftableItem::ClothStrips, CraftingRecipe {
            inputs: vec![(ResourceType::Fabric, 1)],
            output: (CraftableItem::ClothStrips, 4),
            crafting_time: 1.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Conveyor Belt (manual assembly)
        recipes.insert(CraftableItem::ConveyorBelt, CraftingRecipe {
            inputs: vec![(ResourceType::MetalRods, 2), (ResourceType::IronPlates, 1), (ResourceType::IronGears, 1), (ResourceType::ClothStrips, 2)],
            output: (CraftableItem::ConveyorBelt, 1),
            crafting_time: 5.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        Self { recipes }
    }
    
    pub fn can_craft(&self, item: &CraftableItem, inventory: &Inventory) -> bool {
        if let Some(recipe) = self.recipes.get(item) {
            // Check if we have all required resources
            for (resource_type, required_amount) in &recipe.inputs {
                if inventory.get_amount(resource_type) < *required_amount {
                    return false;
                }
            }
            true
        } else {
            false
        }
    }
    
    pub fn get_craftable_items(&self) -> Vec<CraftableItem> {
        self.recipes.keys().cloned().collect()
    }
    
    pub fn get_craft_status(&self, item: &CraftableItem, inventory: &Inventory) -> CraftStatus {
        if let Some(recipe) = self.recipes.get(item) {
            // Check structure requirement
            if let Some(structure) = &recipe.requires_structure {
                if *structure != StructureType::Manual {
                    return CraftStatus::NeedsStructure(format!("Requires {}", structure.get_name()));
                }
            }
            
            // Check resources
            for (resource_type, required_amount) in &recipe.inputs {
                let available = inventory.get_amount(resource_type);
                if available < *required_amount {
                    return CraftStatus::MissingResources(format!("Need {} {} (have {})", required_amount, resource_type.get_name(), available));
                }
            }
            
            CraftStatus::CanCraft
        } else {
            CraftStatus::NoRecipe
        }
    }
}

#[derive(Clone, Debug)]
pub enum CraftStatus {
    CanCraft,
    MissingResources(String),
    NeedsStructure(String),
    NoRecipe,
}

impl StructureType {
    pub fn get_name(&self) -> &'static str {
        match self {
            StructureType::CharcoalPit => "Charcoal Pit",
            StructureType::BloomeryFurnace => "Bloomery Furnace",
            StructureType::StoneAnvil => "Stone Anvil",
            StructureType::SpinningWheel => "Spinning Wheel",
            StructureType::WeavingMachine => "Weaving Machine",
            StructureType::Manual => "Manual",
        }
    }
}
