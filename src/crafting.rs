use crate::types::*;
use crate::world::World;
use raylib::prelude::*;
use std::collections::HashMap;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum CraftingCategory {
    Tools,
    Materials,
    Metals,
    Textiles,
    Food,
    Power,
    Buildings,
    Automation,
    Consumables,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum CraftableItem {
    // Basic materials
    Charcoal,
    
    // Woodworking
    WoodenPlanks,
    WoodenBeams,
    WoodenGears,
    WoodenFrames,
    WoodenRollers,
    
    // Metallurgy
    IronBloom,
    WroughtIron,
    IronPlates,
    IronGears,
    MetalRods,
    SteelIngots,
    SteelPlates,
    
    // Copper working
    CopperIngots,
    CopperPlates,
    CopperWire,
    CopperCoils,
    CopperPipes,
    BronzeAlloy,
    ElectricalComponents,
    
    // Textiles
    Threads,
    Fabric,
    ClothStrips,
    ReinforcedFabric,
    
    // Food production
    Flour,
    Dough,
    Bread,
    Scythe,
    
    // Tools
    WoodenPickaxe,
    StonePickaxe,
    IronPickaxe,
    WoodenAxe,
    StoneAxe,
    IronAxe,
    WoodenShovel,
    StoneShovel,
    IronShovel,
    WoodenSword,
    StoneSword,
    IronSword,
    
    // Animal products
    CookedChicken,
    
    // Steam systems
    WaterBucket,
    SteamPipes,
    PressureValve,
    SteamBoiler,
    SteamDistributionHub,
    
    // Structures
    Campfire,
    CharcoalPit,
    CrudeFurnace,
    BloomeryFurnace,
    StoneAnvil,
    SpinningWheel,
    WeavingMachine,
    AdvancedForge,
    WheatFarm,
    Windmill,
    WaterMill,
    StoneOven,
    GrainSilo,
    WaterPump,
    
    // Automation
    BasicConveyorBelt,
    ReinforcedConveyor,
    SteamConveyor,
    ElectricConveyor,
    SteamPump,
    SteamHammer,
    SortingMachine,
    SteamEngine,
    PowerCables,
    
    // Keep for compatibility
    ConveyorBelt,
}

#[derive(Clone, Debug)]
pub struct CraftingRecipe {
    pub inputs: Vec<(ResourceType, u32)>,
    pub output: (ResourceType, u32),
    pub crafting_time: f32, // In seconds
    pub requires_structure: Option<StructureType>, // What structure is needed to craft this
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum StructureType {
    Campfire,
    CharcoalPit,
    CrudeFurnace,
    BloomeryFurnace,
    StoneAnvil,
    SpinningWheel,
    WeavingMachine,
    AdvancedForge,
    WheatFarm,
    Windmill,
    WaterMill,
    StoneOven,
    GrainSilo,
    SteamBoiler,
    SteamDistributionHub,
    WaterPump,
    SteamPump,
    SteamHammer,
    SortingMachine,
    SteamEngine,
    Manual, // Can be crafted by hand
}

impl CraftingCategory {
    pub fn get_name(&self) -> &'static str {
        match self {
            CraftingCategory::Tools => "Tools",
            CraftingCategory::Materials => "Materials",
            CraftingCategory::Metals => "Metals",
            CraftingCategory::Textiles => "Textiles",
            CraftingCategory::Food => "Food",
            CraftingCategory::Power => "Power",
            CraftingCategory::Buildings => "Buildings",
            CraftingCategory::Automation => "Automation",
            CraftingCategory::Consumables => "Consumables",
        }
    }
    
    pub fn get_items(&self) -> Vec<CraftableItem> {
        match self {
            CraftingCategory::Tools => vec![
                // Basic wooden tools (available from start)
                CraftableItem::WoodenSword,
                CraftableItem::WoodenPickaxe,
                CraftableItem::WoodenAxe,
                CraftableItem::WoodenShovel,
                // Stone tools (early progression)
                CraftableItem::StoneSword,
                CraftableItem::StonePickaxe,
                CraftableItem::StoneAxe,
                CraftableItem::StoneShovel,
                // Iron tools (advanced progression)
                CraftableItem::IronSword,
                CraftableItem::IronPickaxe,
                CraftableItem::IronAxe,
                CraftableItem::IronShovel,
                // Specialized tools
                CraftableItem::Scythe,
            ],
            CraftingCategory::Materials => vec![
                // Basic materials
                CraftableItem::Charcoal,
                // Wood processing
                CraftableItem::WoodenPlanks,
                CraftableItem::WoodenBeams,
                CraftableItem::WoodenGears,
                CraftableItem::WoodenFrames,
                CraftableItem::WoodenRollers,
            ],
            CraftingCategory::Metals => vec![
                // Iron processing
                CraftableItem::IronBloom,
                CraftableItem::WroughtIron,
                CraftableItem::IronPlates,
                CraftableItem::IronGears,
                CraftableItem::MetalRods,
                CraftableItem::SteelIngots,
                CraftableItem::SteelPlates,
                // Copper processing
                CraftableItem::CopperIngots,
                CraftableItem::CopperPlates,
                CraftableItem::CopperWire,
                CraftableItem::CopperCoils,
                CraftableItem::CopperPipes,
                CraftableItem::BronzeAlloy,
                CraftableItem::ElectricalComponents,
            ],
            CraftingCategory::Textiles => vec![
                CraftableItem::Threads,
                CraftableItem::Fabric,
                CraftableItem::ClothStrips,
                CraftableItem::ReinforcedFabric,
            ],
            CraftingCategory::Food => vec![
                CraftableItem::Flour,
                CraftableItem::Dough,
                CraftableItem::Bread,
            ],
            CraftingCategory::Power => vec![
                // Steam systems
                CraftableItem::WaterBucket,
                CraftableItem::SteamPipes,
                CraftableItem::PressureValve,
                CraftableItem::SteamBoiler,
                CraftableItem::SteamDistributionHub,
                // Steam machinery
                CraftableItem::SteamPump,
                CraftableItem::SteamHammer,
                CraftableItem::SteamEngine,
                CraftableItem::PowerCables,
            ],
            CraftingCategory::Buildings => vec![
                // Basic structures (available from start)
                CraftableItem::Campfire,
                CraftableItem::CharcoalPit,
                CraftableItem::CrudeFurnace,
                CraftableItem::StoneAnvil,
                // Intermediate structures
                CraftableItem::BloomeryFurnace,
                CraftableItem::SpinningWheel,
                CraftableItem::WeavingMachine,
                CraftableItem::StoneOven,
                CraftableItem::WaterPump,
                // Advanced structures
                CraftableItem::AdvancedForge,
                CraftableItem::WheatFarm,
                CraftableItem::Windmill,
                CraftableItem::WaterMill,
                CraftableItem::GrainSilo,
            ],
            CraftingCategory::Automation => vec![
                // Conveyor systems
                CraftableItem::BasicConveyorBelt,
                CraftableItem::ReinforcedConveyor,
                CraftableItem::SteamConveyor,
                CraftableItem::ElectricConveyor,
                CraftableItem::ConveyorBelt,
                // Processing machines
                CraftableItem::SortingMachine,
            ],
            CraftingCategory::Consumables => vec![
                // Animal products
                CraftableItem::CookedChicken,
            ],
        }
    }
}

impl CraftableItem {
    pub fn get_category(&self) -> CraftingCategory {
        match self {
            // Tools
            CraftableItem::WoodenPickaxe | CraftableItem::StonePickaxe | CraftableItem::IronPickaxe |
            CraftableItem::WoodenAxe | CraftableItem::StoneAxe | CraftableItem::IronAxe |
            CraftableItem::WoodenShovel | CraftableItem::StoneShovel | CraftableItem::IronShovel |
            CraftableItem::WoodenSword | CraftableItem::StoneSword | CraftableItem::IronSword |
            CraftableItem::Scythe => CraftingCategory::Tools,
            
            // Materials
            CraftableItem::Charcoal | CraftableItem::WoodenPlanks | CraftableItem::WoodenBeams | 
            CraftableItem::WoodenGears | CraftableItem::WoodenFrames | CraftableItem::WoodenRollers => CraftingCategory::Materials,
            
            // Metals
            CraftableItem::IronBloom | CraftableItem::WroughtIron | CraftableItem::IronPlates | 
            CraftableItem::IronGears | CraftableItem::MetalRods | CraftableItem::SteelIngots | 
            CraftableItem::SteelPlates | CraftableItem::CopperIngots | CraftableItem::CopperPlates | 
            CraftableItem::CopperWire | CraftableItem::CopperCoils | CraftableItem::CopperPipes | 
            CraftableItem::BronzeAlloy | CraftableItem::ElectricalComponents => CraftingCategory::Metals,
            
            // Textiles
            CraftableItem::Threads | CraftableItem::Fabric | CraftableItem::ClothStrips | 
            CraftableItem::ReinforcedFabric => CraftingCategory::Textiles,
            
            // Food
            CraftableItem::Flour | CraftableItem::Dough | CraftableItem::Bread => CraftingCategory::Food,
            
            // Power
            CraftableItem::WaterBucket | CraftableItem::SteamPipes | CraftableItem::PressureValve | 
            CraftableItem::SteamBoiler | CraftableItem::SteamDistributionHub | CraftableItem::SteamPump | 
            CraftableItem::SteamHammer | CraftableItem::SteamEngine | CraftableItem::PowerCables => CraftingCategory::Power,
            
            // Buildings
            CraftableItem::Campfire | CraftableItem::CharcoalPit | CraftableItem::CrudeFurnace | 
            CraftableItem::BloomeryFurnace | CraftableItem::StoneAnvil | CraftableItem::SpinningWheel | 
            CraftableItem::WeavingMachine | CraftableItem::AdvancedForge | CraftableItem::WheatFarm | 
            CraftableItem::Windmill | CraftableItem::WaterMill | CraftableItem::StoneOven | 
            CraftableItem::GrainSilo | CraftableItem::WaterPump => CraftingCategory::Buildings,
            
            // Automation
            CraftableItem::BasicConveyorBelt | CraftableItem::ReinforcedConveyor | CraftableItem::SteamConveyor | 
            CraftableItem::ElectricConveyor | CraftableItem::ConveyorBelt | CraftableItem::SortingMachine => CraftingCategory::Automation,
            
            // Consumables
            CraftableItem::CookedChicken => CraftingCategory::Consumables,
        }
    }

    pub fn get_name(&self) -> &'static str {
        match self {
            CraftableItem::Charcoal => "Charcoal",
            
            CraftableItem::WoodenPlanks => "Wooden Planks",
            CraftableItem::WoodenBeams => "Wooden Beams",
            CraftableItem::WoodenGears => "Wooden Gears",
            CraftableItem::WoodenFrames => "Wooden Frames",
            CraftableItem::WoodenRollers => "Wooden Rollers",
            
            CraftableItem::IronBloom => "Iron Bloom",
            CraftableItem::WroughtIron => "Wrought Iron",
            CraftableItem::IronPlates => "Iron Plates",
            CraftableItem::IronGears => "Iron Gears",
            CraftableItem::MetalRods => "Metal Rods",
            CraftableItem::SteelIngots => "Steel Ingots",
            CraftableItem::SteelPlates => "Steel Plates",
            
            CraftableItem::CopperIngots => "Copper Ingots",
            CraftableItem::CopperPlates => "Copper Plates",
            CraftableItem::CopperWire => "Copper Wire",
            CraftableItem::CopperCoils => "Copper Coils",
            CraftableItem::CopperPipes => "Copper Pipes",
            CraftableItem::BronzeAlloy => "Bronze Alloy",
            CraftableItem::ElectricalComponents => "Electrical Components",
            
            CraftableItem::Threads => "Threads",
            CraftableItem::Fabric => "Fabric",
            CraftableItem::ClothStrips => "Cloth Strips",
            CraftableItem::ReinforcedFabric => "Reinforced Fabric",
            
            CraftableItem::Flour => "Flour",
            CraftableItem::Dough => "Dough",
            CraftableItem::Bread => "Bread",
            CraftableItem::Scythe => "Scythe",
            
            CraftableItem::WoodenPickaxe => "Wooden Pickaxe",
            CraftableItem::StonePickaxe => "Stone Pickaxe",
            CraftableItem::IronPickaxe => "Iron Pickaxe",
            CraftableItem::WoodenAxe => "Wooden Axe",
            CraftableItem::StoneAxe => "Stone Axe",
            CraftableItem::IronAxe => "Iron Axe",
            CraftableItem::WoodenShovel => "Wooden Shovel",
            CraftableItem::StoneShovel => "Stone Shovel",
            CraftableItem::IronShovel => "Iron Shovel",
            CraftableItem::WoodenSword => "Wooden Sword",
            CraftableItem::StoneSword => "Stone Sword",
            CraftableItem::IronSword => "Iron Sword",
            
            CraftableItem::CookedChicken => "Cooked Chicken",
            
            CraftableItem::WaterBucket => "Water Bucket",
            CraftableItem::SteamPipes => "Steam Pipes",
            CraftableItem::PressureValve => "Pressure Valve",
            CraftableItem::SteamBoiler => "Steam Boiler",
            CraftableItem::SteamDistributionHub => "Steam Distribution Hub",
            
            CraftableItem::Campfire => "Campfire",
            CraftableItem::CharcoalPit => "Charcoal Pit",
            CraftableItem::CrudeFurnace => "Crude Furnace",
            CraftableItem::BloomeryFurnace => "Bloomery Furnace",
            CraftableItem::StoneAnvil => "Stone Anvil",
            CraftableItem::SpinningWheel => "Spinning Wheel",
            CraftableItem::WeavingMachine => "Weaving Machine",
            CraftableItem::AdvancedForge => "Advanced Forge",
            CraftableItem::WheatFarm => "Wheat Farm",
            CraftableItem::Windmill => "Windmill",
            CraftableItem::WaterMill => "Water Mill",
            CraftableItem::StoneOven => "Stone Oven",
            CraftableItem::GrainSilo => "Grain Silo",
            CraftableItem::WaterPump => "Water Pump",
            
            CraftableItem::BasicConveyorBelt => "Basic Conveyor Belt",
            CraftableItem::ReinforcedConveyor => "Reinforced Conveyor",
            CraftableItem::SteamConveyor => "Steam Conveyor",
            CraftableItem::ElectricConveyor => "Electric Conveyor",
            CraftableItem::SteamPump => "Steam Pump",
            CraftableItem::SteamHammer => "Steam Hammer",
            CraftableItem::SortingMachine => "Sorting Machine",
            CraftableItem::SteamEngine => "Steam Engine",
            CraftableItem::PowerCables => "Power Cables",
            CraftableItem::ConveyorBelt => "Conveyor Belt",
        }
    }
    
    pub fn get_icon_filename(&self) -> &'static str {
        match self {
            CraftableItem::Charcoal => "icons/charcoal.png",
            
            CraftableItem::WoodenPlanks => "icons/wooden_planks.png",
            CraftableItem::WoodenBeams => "icons/wooden_beams.png",
            CraftableItem::WoodenGears => "icons/wooden_gears.png",
            CraftableItem::WoodenFrames => "icons/wooden_frames.png",
            CraftableItem::WoodenRollers => "icons/wooden_rollers.png",
            
            CraftableItem::IronBloom => "icons/iron_bloom.png",
            CraftableItem::WroughtIron => "icons/wrought_iron.png",
            CraftableItem::IronPlates => "icons/iron_plates.png",
            CraftableItem::IronGears => "icons/iron_gears.png",
            CraftableItem::MetalRods => "icons/metal_rods.png",
            CraftableItem::SteelIngots => "icons/steel_ingots.png",
            CraftableItem::SteelPlates => "icons/steel_plates.png",
            
            CraftableItem::CopperIngots => "icons/copper_ingots.png",
            CraftableItem::CopperPlates => "icons/copper_plates.png",
            CraftableItem::CopperWire => "icons/copper_wire.png",
            CraftableItem::CopperCoils => "icons/copper_coils.png",
            CraftableItem::CopperPipes => "icons/copper_pipes.png",
            CraftableItem::BronzeAlloy => "icons/bronze_alloy.png",
            CraftableItem::ElectricalComponents => "icons/electrical_components.png",
            
            CraftableItem::Threads => "icons/threads.png",
            CraftableItem::Fabric => "icons/fabric.png",
            CraftableItem::ClothStrips => "icons/cloth_strips.png",
            CraftableItem::ReinforcedFabric => "icons/reinforced_fabric.png",
            
            CraftableItem::Flour => "icons/flour.png",
            CraftableItem::Dough => "icons/dough.png",
            CraftableItem::Bread => "icons/bread.png",
            CraftableItem::Scythe => "icons/scythe.png",
            
            CraftableItem::WoodenPickaxe => "icons/wooden_pickaxe.png",
            CraftableItem::StonePickaxe => "icons/stone_pickaxe.png",
            CraftableItem::IronPickaxe => "icons/iron_pickaxe.png",
            CraftableItem::WoodenAxe => "icons/wooden_axe.png",
            CraftableItem::StoneAxe => "icons/stone_axe.png",
            CraftableItem::IronAxe => "icons/iron_axe.png",
            CraftableItem::WoodenShovel => "icons/wooden_shovel.png",
            CraftableItem::StoneShovel => "icons/stone_shovel.png",
            CraftableItem::IronShovel => "icons/iron_shovel.png",
            CraftableItem::WoodenSword => "icons/wooden_sword.png",
            CraftableItem::StoneSword => "icons/stone_sword.png",
            CraftableItem::IronSword => "icons/iron_sword.png",
            
            CraftableItem::CookedChicken => "icons/cooked_chicken.png",
            
            CraftableItem::WaterBucket => "icons/water_bucket.png",
            CraftableItem::SteamPipes => "icons/steam_pipes.png",
            CraftableItem::PressureValve => "icons/pressure_valve.png",
            CraftableItem::SteamBoiler => "icons/steam_boiler.png",
            CraftableItem::SteamDistributionHub => "icons/steam_distribution_hub.png",
            
            CraftableItem::Campfire => "icons/campfire.png",
            CraftableItem::CharcoalPit => "icons/charcoal_pit.png",
            CraftableItem::CrudeFurnace => "icons/crude_furnace.png",
            CraftableItem::BloomeryFurnace => "icons/bloomery_furnace.png",
            CraftableItem::StoneAnvil => "icons/stone_anvil.png",
            CraftableItem::SpinningWheel => "icons/spinning_wheel.png",
            CraftableItem::WeavingMachine => "icons/weaving_machine.png",
            CraftableItem::AdvancedForge => "icons/advanced_forge.png",
            CraftableItem::WheatFarm => "icons/wheat_farm.png",
            CraftableItem::Windmill => "icons/windmill.png",
            CraftableItem::WaterMill => "icons/water_mill.png",
            CraftableItem::StoneOven => "icons/stone_oven.png",
            CraftableItem::GrainSilo => "icons/grain_silo.png",
            CraftableItem::WaterPump => "icons/water_pump.png",
            
            CraftableItem::BasicConveyorBelt => "icons/basic_conveyor_belt.png",
            CraftableItem::ReinforcedConveyor => "icons/reinforced_conveyor.png",
            CraftableItem::SteamConveyor => "icons/steam_conveyor.png",
            CraftableItem::ElectricConveyor => "icons/electric_conveyor.png",
            CraftableItem::SteamPump => "icons/steam_pump.png",
            CraftableItem::SteamHammer => "icons/steam_hammer.png",
            CraftableItem::SortingMachine => "icons/sorting_machine.png",
            CraftableItem::SteamEngine => "icons/steam_engine.png",
            CraftableItem::PowerCables => "icons/power_cables.png",
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
        
        // === BASIC MATERIALS ===
        // Charcoal (requires Charcoal Pit + Wood)
        recipes.insert(CraftableItem::Charcoal, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 3)],
            output: (ResourceType::Charcoal, 2),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::CharcoalPit),
        });
        
        // === WOODWORKING ===
        // Wooden Planks (manual from Wood)
        recipes.insert(CraftableItem::WoodenPlanks, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 2)],
            output: (ResourceType::WoodenPlanks, 4),
            crafting_time: 2.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Wooden Beams (manual from Wooden Planks)
        recipes.insert(CraftableItem::WoodenBeams, CraftingRecipe {
            inputs: vec![(ResourceType::WoodenPlanks, 3)],
            output: (ResourceType::WoodenBeams, 2),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Wooden Gears (manual from Wooden Planks)
        recipes.insert(CraftableItem::WoodenGears, CraftingRecipe {
            inputs: vec![(ResourceType::WoodenPlanks, 2)],
            output: (ResourceType::WoodenGears, 1),
            crafting_time: 2.5,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Wooden Frames (manual from Wooden Beams + Planks)
        recipes.insert(CraftableItem::WoodenFrames, CraftingRecipe {
            inputs: vec![(ResourceType::WoodenBeams, 2), (ResourceType::WoodenPlanks, 1)],
            output: (ResourceType::WoodenFrames, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Wooden Rollers (manual from Wooden Beams + Metal Rods)
        recipes.insert(CraftableItem::WoodenRollers, CraftingRecipe {
            inputs: vec![(ResourceType::WoodenBeams, 1), (ResourceType::MetalRods, 1)],
            output: (ResourceType::WoodenRollers, 2),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // === METALLURGY ===
        // Iron Bloom (requires Bloomery Furnace + Coal + Iron Ore)
        recipes.insert(CraftableItem::IronBloom, CraftingRecipe {
            inputs: vec![(ResourceType::IronOre, 2), (ResourceType::Coal, 1)],
            output: (ResourceType::IronBloom, 1),
            crafting_time: 6.0,
            requires_structure: Some(StructureType::BloomeryFurnace),
        });
        
        // Wrought Iron (requires Stone Anvil + Iron Bloom)
        recipes.insert(CraftableItem::WroughtIron, CraftingRecipe {
            inputs: vec![(ResourceType::IronBloom, 1)],
            output: (ResourceType::WroughtIron, 1),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::StoneAnvil),
        });
        
        // Iron Plates (requires Stone Anvil + Wrought Iron)
        recipes.insert(CraftableItem::IronPlates, CraftingRecipe {
            inputs: vec![(ResourceType::WroughtIron, 1)],
            output: (ResourceType::IronPlates, 2),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::StoneAnvil),
        });
        
        // Iron Gears (manual crafting from Iron Plates)
        recipes.insert(CraftableItem::IronGears, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 2)],
            output: (ResourceType::IronGears, 1),
            crafting_time: 2.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Metal Rods (manual crafting from Iron Plates)
        recipes.insert(CraftableItem::MetalRods, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 1)],
            output: (ResourceType::MetalRods, 2),
            crafting_time: 1.5,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Steel Ingots (requires Advanced Forge + Iron Plates + Charcoal)
        recipes.insert(CraftableItem::SteelIngots, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 2), (ResourceType::Charcoal, 1)],
            output: (ResourceType::SteelIngots, 1),
            crafting_time: 8.0,
            requires_structure: Some(StructureType::AdvancedForge),
        });
        
        // Steel Plates (requires Advanced Forge + Steel Ingots)
        recipes.insert(CraftableItem::SteelPlates, CraftingRecipe {
            inputs: vec![(ResourceType::SteelIngots, 1)],
            output: (ResourceType::SteelPlates, 2),
            crafting_time: 5.0,
            requires_structure: Some(StructureType::AdvancedForge),
        });
        
        // === COPPER WORKING ===
        // Copper Ingots (requires Crude Furnace + Copper Ore + Charcoal)
        recipes.insert(CraftableItem::CopperIngots, CraftingRecipe {
            inputs: vec![(ResourceType::CopperOre, 2), (ResourceType::Charcoal, 1)],
            output: (ResourceType::CopperIngots, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::CrudeFurnace),
        });
        
        // Copper Plates (requires Stone Anvil + Copper Ingots)
        recipes.insert(CraftableItem::CopperPlates, CraftingRecipe {
            inputs: vec![(ResourceType::CopperIngots, 1)],
            output: (ResourceType::CopperPlates, 3),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::StoneAnvil),
        });
        
        // Copper Wire (manual from Copper Plates)
        recipes.insert(CraftableItem::CopperWire, CraftingRecipe {
            inputs: vec![(ResourceType::CopperPlates, 1)],
            output: (ResourceType::CopperWire, 4),
            crafting_time: 2.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Copper Coils (manual from Copper Wire)
        recipes.insert(CraftableItem::CopperCoils, CraftingRecipe {
            inputs: vec![(ResourceType::CopperWire, 3)],
            output: (ResourceType::CopperCoils, 1),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Copper Pipes (manual from Copper Plates)
        recipes.insert(CraftableItem::CopperPipes, CraftingRecipe {
            inputs: vec![(ResourceType::CopperPlates, 1)],
            output: (ResourceType::CopperPipes, 2),
            crafting_time: 2.5,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Bronze Alloy (manual from Copper Plates + Iron Plates)
        recipes.insert(CraftableItem::BronzeAlloy, CraftingRecipe {
            inputs: vec![(ResourceType::CopperPlates, 2), (ResourceType::IronPlates, 1)],
            output: (ResourceType::BronzeAlloy, 2),
            crafting_time: 6.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Electrical Components (manual from Copper Wire + Iron Gears)
        recipes.insert(CraftableItem::ElectricalComponents, CraftingRecipe {
            inputs: vec![(ResourceType::CopperWire, 2), (ResourceType::IronGears, 1)],
            output: (ResourceType::ElectricalComponents, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // === TEXTILES ===
        // Threads (requires Spinning Wheel + Cotton)
        recipes.insert(CraftableItem::Threads, CraftingRecipe {
            inputs: vec![(ResourceType::Cotton, 2)],
            output: (ResourceType::Threads, 3),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::SpinningWheel),
        });
        
        // Fabric (requires Weaving Machine + Threads)
        recipes.insert(CraftableItem::Fabric, CraftingRecipe {
            inputs: vec![(ResourceType::Threads, 3)],
            output: (ResourceType::Fabric, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::WeavingMachine),
        });
        
        // Cloth Strips (manual cutting from Fabric)
        recipes.insert(CraftableItem::ClothStrips, CraftingRecipe {
            inputs: vec![(ResourceType::Fabric, 1)],
            output: (ResourceType::ClothStrips, 4),
            crafting_time: 1.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Reinforced Fabric (manual from Fabric + Copper Wire)
        recipes.insert(CraftableItem::ReinforcedFabric, CraftingRecipe {
            inputs: vec![(ResourceType::Fabric, 2), (ResourceType::CopperWire, 1)],
            output: (ResourceType::ReinforcedFabric, 1),
            crafting_time: 5.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // === FOOD PRODUCTION ===
        // Flour (requires Windmill + Wheat)
        recipes.insert(CraftableItem::Flour, CraftingRecipe {
            inputs: vec![(ResourceType::Wheat, 3)],
            output: (ResourceType::Flour, 2),
            crafting_time: 5.0,
            requires_structure: Some(StructureType::Windmill),
        });
        
        // Dough (manual from Flour + Water)
        recipes.insert(CraftableItem::Dough, CraftingRecipe {
            inputs: vec![(ResourceType::Flour, 2), (ResourceType::Water, 1)],
            output: (ResourceType::Dough, 1),
            crafting_time: 2.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Bread (requires Stone Oven + Dough)
        recipes.insert(CraftableItem::Bread, CraftingRecipe {
            inputs: vec![(ResourceType::Dough, 1)],
            output: (ResourceType::Bread, 2),
            crafting_time: 8.0,
            requires_structure: Some(StructureType::StoneOven),
        });
        
        // Scythe (manual from Wood + Metal Rods + Iron Plates)
        recipes.insert(CraftableItem::Scythe, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 3), (ResourceType::MetalRods, 2), (ResourceType::IronPlates, 1)],
            output: (ResourceType::Scythe, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // === BASIC TOOLS ===
        // Wooden Pickaxe (manual from Sticks + Twigs for binding)
        recipes.insert(CraftableItem::WoodenPickaxe, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::Twigs, 2)],
            output: (ResourceType::WoodenPickaxe, 1),
            crafting_time: 2.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Stone Pickaxe (manual from Sticks + Rocks + Plant Fiber)
        recipes.insert(CraftableItem::StonePickaxe, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::Rocks, 3), (ResourceType::PlantFiber, 1)],
            output: (ResourceType::StonePickaxe, 1),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Iron Pickaxe (manual from Sticks + Iron Plates)
        recipes.insert(CraftableItem::IronPickaxe, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::IronPlates, 2)],
            output: (ResourceType::IronPickaxe, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Wooden Axe (manual from Sticks + Flint + Twigs for binding)
        recipes.insert(CraftableItem::WoodenAxe, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::Flint, 1), (ResourceType::Twigs, 1)],
            output: (ResourceType::WoodenAxe, 1),
            crafting_time: 2.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Stone Axe (manual from Sticks + Rocks + Plant Fiber)
        recipes.insert(CraftableItem::StoneAxe, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::Rocks, 2), (ResourceType::PlantFiber, 1)],
            output: (ResourceType::StoneAxe, 1),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Iron Axe (manual from Sticks + Iron Plates)
        recipes.insert(CraftableItem::IronAxe, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::IronPlates, 2)],
            output: (ResourceType::IronAxe, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Wooden Shovel (manual from Sticks + Twigs for binding)
        recipes.insert(CraftableItem::WoodenShovel, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::Twigs, 3)],
            output: (ResourceType::WoodenShovel, 1),
            crafting_time: 2.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Stone Shovel (manual from Sticks + Rocks + Plant Fiber)
        recipes.insert(CraftableItem::StoneShovel, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::Rocks, 2), (ResourceType::PlantFiber, 1)],
            output: (ResourceType::StoneShovel, 1),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Iron Shovel (manual from Sticks + Iron Plates)
        recipes.insert(CraftableItem::IronShovel, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::IronPlates, 2)],
            output: (ResourceType::IronShovel, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // === WEAPONS ===
        // Wooden Sword (manual from Sticks + Twigs for binding)
        recipes.insert(CraftableItem::WoodenSword, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::Twigs, 1)],
            output: (ResourceType::WoodenSword, 1),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Stone Sword (manual from Sticks + Rocks + Plant Fiber)
        recipes.insert(CraftableItem::StoneSword, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::Rocks, 2), (ResourceType::PlantFiber, 1)],
            output: (ResourceType::StoneSword, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Iron Sword (manual from Sticks + Iron Plates)
        recipes.insert(CraftableItem::IronSword, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 2), (ResourceType::IronPlates, 3)],
            output: (ResourceType::IronSword, 1),
            crafting_time: 5.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // === ANIMAL PRODUCTS ===
        // Cooked Chicken (requires Campfire + Raw Chicken)
        recipes.insert(CraftableItem::CookedChicken, CraftingRecipe {
            inputs: vec![(ResourceType::RawChicken, 1)],
            output: (ResourceType::CookedChicken, 1),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::Campfire),
        });
        
        // === STEAM SYSTEMS ===
        // Water Bucket (manual from Metal Rods + Iron Plates)
        recipes.insert(CraftableItem::WaterBucket, CraftingRecipe {
            inputs: vec![(ResourceType::MetalRods, 2), (ResourceType::IronPlates, 1)],
            output: (ResourceType::WaterBucket, 1),
            crafting_time: 2.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Steam Pipes (manual from Copper Pipes + Iron Plates)
        recipes.insert(CraftableItem::SteamPipes, CraftingRecipe {
            inputs: vec![(ResourceType::CopperPipes, 2), (ResourceType::IronPlates, 1)],
            output: (ResourceType::SteamPipes, 4),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Pressure Valve (manual from Iron Plates + Copper Plates + Iron Gears)
        recipes.insert(CraftableItem::PressureValve, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 2), (ResourceType::CopperPlates, 1), (ResourceType::IronGears, 1)],
            output: (ResourceType::PressureValve, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Steam Boiler (manual from Iron Plates + Copper Pipes + Metal Rods)
        recipes.insert(CraftableItem::SteamBoiler, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 6), (ResourceType::CopperPipes, 4), (ResourceType::MetalRods, 2)],
            output: (ResourceType::SteamBoiler, 1),
            crafting_time: 15.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Steam Distribution Hub (manual from Iron Plates + Steam Pipes + Pressure Valve)
        recipes.insert(CraftableItem::SteamDistributionHub, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 4), (ResourceType::SteamPipes, 8), (ResourceType::PressureValve, 1)],
            output: (ResourceType::SteamDistributionHub, 1),
            crafting_time: 10.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // === STRUCTURES ===
        // Campfire (manual from Sticks + Rocks)
        recipes.insert(CraftableItem::Campfire, CraftingRecipe {
            inputs: vec![(ResourceType::Sticks, 5), (ResourceType::Rocks, 3)],
            output: (ResourceType::Campfire, 1),
            crafting_time: 2.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Charcoal Pit (manual from Stone + Clay)
        recipes.insert(CraftableItem::CharcoalPit, CraftingRecipe {
            inputs: vec![(ResourceType::Stone, 10), (ResourceType::Clay, 5)],
            output: (ResourceType::CharcoalPit, 1),
            crafting_time: 5.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Crude Furnace (manual from Stone + Clay)
        recipes.insert(CraftableItem::CrudeFurnace, CraftingRecipe {
            inputs: vec![(ResourceType::Stone, 8), (ResourceType::Clay, 6)],
            output: (ResourceType::CrudeFurnace, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Bloomery Furnace (manual from Stone + Clay)
        recipes.insert(CraftableItem::BloomeryFurnace, CraftingRecipe {
            inputs: vec![(ResourceType::Stone, 15), (ResourceType::Clay, 8)],
            output: (ResourceType::BloomeryFurnace, 1),
            crafting_time: 8.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Stone Anvil (manual from Stone)
        recipes.insert(CraftableItem::StoneAnvil, CraftingRecipe {
            inputs: vec![(ResourceType::Stone, 20)],
            output: (ResourceType::StoneAnvil, 1),
            crafting_time: 6.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Spinning Wheel (manual from Wood)
        recipes.insert(CraftableItem::SpinningWheel, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 25)],
            output: (ResourceType::SpinningWheel, 1),
            crafting_time: 10.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Weaving Machine (manual from Wood)
        recipes.insert(CraftableItem::WeavingMachine, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 35)],
            output: (ResourceType::WeavingMachine, 1),
            crafting_time: 15.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Advanced Forge (manual from Stone + Iron Plates + Copper Plates)
        recipes.insert(CraftableItem::AdvancedForge, CraftingRecipe {
            inputs: vec![(ResourceType::Stone, 25), (ResourceType::IronPlates, 8), (ResourceType::CopperPlates, 4)],
            output: (ResourceType::AdvancedForge, 1),
            crafting_time: 20.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Wheat Farm (manual from Wood + Stone + Wheat Seeds + Water)
        recipes.insert(CraftableItem::WheatFarm, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 20), (ResourceType::Stone, 10), (ResourceType::WheatSeeds, 5), (ResourceType::Water, 10)],
            output: (ResourceType::WheatFarm, 1),
            crafting_time: 12.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Windmill (manual from Wood + Stone + Iron Gears + Cloth Strips)
        recipes.insert(CraftableItem::Windmill, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 40), (ResourceType::Stone, 20), (ResourceType::IronGears, 4), (ResourceType::ClothStrips, 8)],
            output: (ResourceType::Windmill, 1),
            crafting_time: 25.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Water Mill (manual from Wood + Stone + Iron Gears + Cloth Strips)
        recipes.insert(CraftableItem::WaterMill, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 35), (ResourceType::Stone, 15), (ResourceType::IronGears, 3), (ResourceType::ClothStrips, 6)],
            output: (ResourceType::WaterMill, 1),
            crafting_time: 20.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Stone Oven (manual from Stone + Clay + Iron Plates)
        recipes.insert(CraftableItem::StoneOven, CraftingRecipe {
            inputs: vec![(ResourceType::Stone, 30), (ResourceType::Clay, 15), (ResourceType::IronPlates, 4)],
            output: (ResourceType::StoneOven, 1),
            crafting_time: 15.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Grain Silo (manual from Wood + Stone + Iron Plates)
        recipes.insert(CraftableItem::GrainSilo, CraftingRecipe {
            inputs: vec![(ResourceType::Wood, 50), (ResourceType::Stone, 20), (ResourceType::IronPlates, 6)],
            output: (ResourceType::GrainSilo, 1),
            crafting_time: 18.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Water Pump (manual from Iron Plates + Copper Pipes + Iron Gears)
        recipes.insert(CraftableItem::WaterPump, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 4), (ResourceType::CopperPipes, 6), (ResourceType::IronGears, 2)],
            output: (ResourceType::WaterPump, 1),
            crafting_time: 12.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // === AUTOMATION ===
        // Basic Conveyor Belt (manual from Wooden Frames + Wooden Rollers + Cloth Strips)
        recipes.insert(CraftableItem::BasicConveyorBelt, CraftingRecipe {
            inputs: vec![(ResourceType::WoodenFrames, 1), (ResourceType::WoodenRollers, 2), (ResourceType::ClothStrips, 3)],
            output: (ResourceType::BasicConveyorBelt, 1),
            crafting_time: 4.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Reinforced Conveyor (manual from Metal Rods + Iron Plates + Iron Gears + Cloth Strips)
        recipes.insert(CraftableItem::ReinforcedConveyor, CraftingRecipe {
            inputs: vec![(ResourceType::MetalRods, 2), (ResourceType::IronPlates, 1), (ResourceType::IronGears, 1), (ResourceType::ClothStrips, 2)],
            output: (ResourceType::ReinforcedConveyor, 1),
            crafting_time: 5.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Steam Conveyor (manual from Basic Conveyor Belt + Steam Pipes + Pressure Valve)
        recipes.insert(CraftableItem::SteamConveyor, CraftingRecipe {
            inputs: vec![(ResourceType::BasicConveyorBelt, 1), (ResourceType::SteamPipes, 2), (ResourceType::PressureValve, 1)],
            output: (ResourceType::SteamConveyor, 1),
            crafting_time: 6.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Electric Conveyor (manual from Reinforced Conveyor + Electrical Components + Copper Wire)
        recipes.insert(CraftableItem::ElectricConveyor, CraftingRecipe {
            inputs: vec![(ResourceType::ReinforcedConveyor, 1), (ResourceType::ElectricalComponents, 1), (ResourceType::CopperWire, 2)],
            output: (ResourceType::ElectricConveyor, 1),
            crafting_time: 8.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Steam Pump (manual from Water Pump + Steam Pipes + Pressure Valve)
        recipes.insert(CraftableItem::SteamPump, CraftingRecipe {
            inputs: vec![(ResourceType::WaterPump, 1), (ResourceType::SteamPipes, 3), (ResourceType::PressureValve, 1)],
            output: (ResourceType::SteamPump, 1),
            crafting_time: 8.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Steam Hammer (manual from Stone Anvil + Steam Pipes + Iron Plates + Pressure Valve)
        recipes.insert(CraftableItem::SteamHammer, CraftingRecipe {
            inputs: vec![(ResourceType::StoneAnvil, 1), (ResourceType::SteamPipes, 4), (ResourceType::IronPlates, 6), (ResourceType::PressureValve, 2)],
            output: (ResourceType::SteamHammer, 1),
            crafting_time: 25.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Sorting Machine (manual from Iron Plates + Copper Plates + Electrical Components + Iron Gears)
        recipes.insert(CraftableItem::SortingMachine, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 6), (ResourceType::CopperPlates, 4), (ResourceType::ElectricalComponents, 2), (ResourceType::IronGears, 3)],
            output: (ResourceType::SortingMachine, 1),
            crafting_time: 15.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Steam Engine (manual from Iron Plates + Copper Pipes + Steel Plates + Iron Gears + Pressure Valve)
        recipes.insert(CraftableItem::SteamEngine, CraftingRecipe {
            inputs: vec![(ResourceType::IronPlates, 8), (ResourceType::CopperPipes, 6), (ResourceType::SteelPlates, 2), (ResourceType::IronGears, 4), (ResourceType::PressureValve, 2)],
            output: (ResourceType::SteamEngine, 1),
            crafting_time: 20.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Power Cables (manual from Copper Wire + Reinforced Fabric)
        recipes.insert(CraftableItem::PowerCables, CraftingRecipe {
            inputs: vec![(ResourceType::CopperWire, 5), (ResourceType::ReinforcedFabric, 1)],
            output: (ResourceType::PowerCables, 3),
            crafting_time: 3.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        // Keep compatibility with old ConveyorBelt
        recipes.insert(CraftableItem::ConveyorBelt, CraftingRecipe {
            inputs: vec![(ResourceType::MetalRods, 2), (ResourceType::IronPlates, 1), (ResourceType::IronGears, 1), (ResourceType::ClothStrips, 2)],
            output: (ResourceType::ConveyorBelt, 1),
            crafting_time: 5.0,
            requires_structure: Some(StructureType::Manual),
        });
        
        Self { recipes }
    }
    
    // Helper function to check if a building is nearby
    fn has_nearby_building(&self, structure_type: StructureType, player_pos: Vector2, world: &World) -> bool {
        let search_radius = 3; // Search within 3 tiles
        let player_tile_x = (player_pos.x / TILE_SIZE as f32) as i32;
        let player_tile_y = (player_pos.y / TILE_SIZE as f32) as i32;
        
        for dx in -search_radius..=search_radius {
            for dy in -search_radius..=search_radius {
                let tile_x = player_tile_x + dx;
                let tile_y = player_tile_y + dy;
                
                if tile_x >= 0 && tile_y >= 0 && 
                   tile_x < world.width as i32 && tile_y < world.height as i32 {
                    if let Some(tile) = world.get_tile(tile_x as usize, tile_y as usize) {
                        if let Some(building) = tile.building {
                            if self.building_type_to_structure(building) == Some(structure_type) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        false
    }
    
    // Helper function to convert BuildingType to StructureType
    fn building_type_to_structure(&self, building_type: BuildingType) -> Option<StructureType> {
        match building_type {
            BuildingType::Campfire => Some(StructureType::Campfire),
            BuildingType::CharcoalPit => Some(StructureType::CharcoalPit),
            BuildingType::CrudeFurnace => Some(StructureType::CrudeFurnace),
            BuildingType::BloomeryFurnace => Some(StructureType::BloomeryFurnace),
            BuildingType::StoneAnvil => Some(StructureType::StoneAnvil),
            BuildingType::SpinningWheel => Some(StructureType::SpinningWheel),
            BuildingType::WeavingMachine => Some(StructureType::WeavingMachine),
            _ => None,
        }
    }
    
    // Helper function to convert StructureType to ResourceType
    fn structure_to_resource_type(&self, structure_type: StructureType) -> ResourceType {
        match structure_type {
            StructureType::Campfire => ResourceType::Campfire,
            StructureType::CharcoalPit => ResourceType::CharcoalPit,
            StructureType::CrudeFurnace => ResourceType::CrudeFurnace,
            StructureType::BloomeryFurnace => ResourceType::BloomeryFurnace,
            StructureType::StoneAnvil => ResourceType::StoneAnvil,
            StructureType::SpinningWheel => ResourceType::SpinningWheel,
            StructureType::WeavingMachine => ResourceType::WeavingMachine,
            StructureType::AdvancedForge => ResourceType::AdvancedForge,
            StructureType::StoneOven => ResourceType::StoneOven,
            _ => ResourceType::Wood, // Fallback
        }
    }
    
    pub fn get_recipes_for_building(&self, building_type: BuildingType) -> Vec<CraftableItem> {
        let structure_type = self.building_type_to_structure(building_type);
        if structure_type.is_none() {
            return vec![];
        }
        
        let structure = structure_type.unwrap();
        let mut recipes = Vec::new();
        
        for (item, recipe) in &self.recipes {
            if let Some(required_structure) = &recipe.requires_structure {
                if *required_structure == structure {
                    recipes.push(*item);
                }
            }
        }
        
        recipes
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
    
    pub fn get_craft_status(&self, item: &CraftableItem, inventory: &Inventory, world: Option<&World>, player_pos: Option<Vector2>) -> CraftStatus {
        if let Some(recipe) = self.recipes.get(item) {
            // Check structure requirement
            if let Some(structure) = &recipe.requires_structure {
                if *structure != StructureType::Manual {
                    // Check if player has the building nearby
                    if let (Some(world), Some(player_pos)) = (world, player_pos) {
                        if self.has_nearby_building(*structure, player_pos, world) {
                            // Building is available - continue to resource check
                        } else {
                            // Check if player has the building in inventory
                            let building_resource = self.structure_to_resource_type(*structure);
                            if inventory.get_amount(&building_resource) > 0 {
                                return CraftStatus::NeedsStructure(format!("Place {}", structure.get_name()));
                            } else {
                                return CraftStatus::BuildingNotAvailable(format!("Need to build {}", structure.get_name()));
                            }
                        }
                    } else {
                        // Fallback to old behavior when world/player pos not available
                        return CraftStatus::NeedsStructure(format!("Requires {}", structure.get_name()));
                    }
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
    BuildingNotAvailable(String),
    NoRecipe,
}

impl StructureType {
    pub fn get_name(&self) -> &'static str {
        match self {
            StructureType::Campfire => "Campfire",
            StructureType::CharcoalPit => "Charcoal Pit",
            StructureType::CrudeFurnace => "Crude Furnace",
            StructureType::BloomeryFurnace => "Bloomery Furnace",
            StructureType::StoneAnvil => "Stone Anvil",
            StructureType::SpinningWheel => "Spinning Wheel",
            StructureType::WeavingMachine => "Weaving Machine",
            StructureType::AdvancedForge => "Advanced Forge",
            StructureType::WheatFarm => "Wheat Farm",
            StructureType::Windmill => "Windmill",
            StructureType::WaterMill => "Water Mill",
            StructureType::StoneOven => "Stone Oven",
            StructureType::GrainSilo => "Grain Silo",
            StructureType::SteamBoiler => "Steam Boiler",
            StructureType::SteamDistributionHub => "Steam Distribution Hub",
            StructureType::WaterPump => "Water Pump",
            StructureType::SteamPump => "Steam Pump",
            StructureType::SteamHammer => "Steam Hammer",
            StructureType::SortingMachine => "Sorting Machine",
            StructureType::SteamEngine => "Steam Engine",
            StructureType::Manual => "Manual",
        }
    }
}
