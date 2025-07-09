use raylib::prelude::*;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum ResourceType {
    // Basic resources
    Wood,
    Stone,
    IronOre,
    Coal,
    Clay,
    CopperOre,
    Cotton,
    Water,
    WheatSeeds,
    Wheat,
    
    // Animal resources
    Egg,
    RawChicken,
    CookedChicken,
    ChickenFeathers,
    
    // Processed materials - Wood
    WoodenPlanks,
    WoodenBeams,
    WoodenGears,
    WoodenFrames,
    WoodenRollers,
    
    // Processed materials - Metal
    Charcoal,
    IronBloom,
    WroughtIron,
    IronPlates,
    IronGears,
    MetalRods,
    SteelIngots,
    SteelPlates,
    
    // Processed materials - Copper
    CopperIngots,
    CopperPlates,
    CopperWire,
    CopperCoils,
    CopperPipes,
    BronzeAlloy,
    ElectricalComponents,
    
    // Processed materials - Textiles
    Threads,
    Fabric,
    ClothStrips,
    ReinforcedFabric,
    
    // Food items
    Flour,
    Dough,
    Bread,
    
    // Steam components
    WaterBucket,
    SteamPipes,
    PressureValve,
    
    // Tools
    Scythe,
    
    // Automation components
    BasicConveyorBelt,
    ReinforcedConveyor,
    SteamConveyor,
    ElectricConveyor,
    PowerCables,
    
    // Buildings/Structures
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
    ConveyorBelt, // Keep for compatibility
}

impl ResourceType {
    pub fn get_name(&self) -> &'static str {
        match self {
            // Basic resources
            ResourceType::Wood => "Wood",
            ResourceType::Stone => "Stone", 
            ResourceType::IronOre => "Iron Ore",
            ResourceType::Coal => "Coal",
            ResourceType::Clay => "Clay",
            ResourceType::CopperOre => "Copper Ore",
            ResourceType::Cotton => "Cotton",
            ResourceType::Water => "Water",
            ResourceType::WheatSeeds => "Wheat Seeds",
            ResourceType::Wheat => "Wheat",
            
            // Animal resources
            ResourceType::Egg => "Egg",
            ResourceType::RawChicken => "Raw Chicken",
            ResourceType::CookedChicken => "Cooked Chicken",
            ResourceType::ChickenFeathers => "Chicken Feathers",
            
            // Wood products
            ResourceType::WoodenPlanks => "Wooden Planks",
            ResourceType::WoodenBeams => "Wooden Beams",
            ResourceType::WoodenGears => "Wooden Gears",
            ResourceType::WoodenFrames => "Wooden Frames",
            ResourceType::WoodenRollers => "Wooden Rollers",
            
            // Metal products
            ResourceType::Charcoal => "Charcoal",
            ResourceType::IronBloom => "Iron Bloom",
            ResourceType::WroughtIron => "Wrought Iron",
            ResourceType::IronPlates => "Iron Plates",
            ResourceType::IronGears => "Iron Gears",
            ResourceType::MetalRods => "Metal Rods",
            ResourceType::SteelIngots => "Steel Ingots",
            ResourceType::SteelPlates => "Steel Plates",
            
            // Copper products
            ResourceType::CopperIngots => "Copper Ingots",
            ResourceType::CopperPlates => "Copper Plates",
            ResourceType::CopperWire => "Copper Wire",
            ResourceType::CopperCoils => "Copper Coils",
            ResourceType::CopperPipes => "Copper Pipes",
            ResourceType::BronzeAlloy => "Bronze Alloy",
            ResourceType::ElectricalComponents => "Electrical Components",
            
            // Textiles
            ResourceType::Threads => "Threads",
            ResourceType::Fabric => "Fabric",
            ResourceType::ClothStrips => "Cloth Strips",
            ResourceType::ReinforcedFabric => "Reinforced Fabric",
            
            // Food
            ResourceType::Flour => "Flour",
            ResourceType::Dough => "Dough",
            ResourceType::Bread => "Bread",
            
            // Steam
            ResourceType::WaterBucket => "Water Bucket",
            ResourceType::SteamPipes => "Steam Pipes",
            ResourceType::PressureValve => "Pressure Valve",
            
            // Tools
            ResourceType::Scythe => "Scythe",
            
            // Automation
            ResourceType::BasicConveyorBelt => "Basic Conveyor Belt",
            ResourceType::ReinforcedConveyor => "Reinforced Conveyor",
            ResourceType::SteamConveyor => "Steam Conveyor",
            ResourceType::ElectricConveyor => "Electric Conveyor",
            ResourceType::PowerCables => "Power Cables",
            
            // Buildings
            ResourceType::CharcoalPit => "Charcoal Pit",
            ResourceType::CrudeFurnace => "Crude Furnace",
            ResourceType::BloomeryFurnace => "Bloomery Furnace",
            ResourceType::StoneAnvil => "Stone Anvil",
            ResourceType::SpinningWheel => "Spinning Wheel",
            ResourceType::WeavingMachine => "Weaving Machine",
            ResourceType::AdvancedForge => "Advanced Forge",
            ResourceType::WheatFarm => "Wheat Farm",
            ResourceType::Windmill => "Windmill",
            ResourceType::WaterMill => "Water Mill",
            ResourceType::StoneOven => "Stone Oven",
            ResourceType::GrainSilo => "Grain Silo",
            ResourceType::SteamBoiler => "Steam Boiler",
            ResourceType::SteamDistributionHub => "Steam Distribution Hub",
            ResourceType::WaterPump => "Water Pump",
            ResourceType::SteamPump => "Steam Pump",
            ResourceType::SteamHammer => "Steam Hammer",
            ResourceType::SortingMachine => "Sorting Machine",
            ResourceType::SteamEngine => "Steam Engine",
            ResourceType::ConveyorBelt => "Conveyor Belt",
        }
    }
    
    pub fn get_color(&self) -> Color {
        match self {
            // Basic resources
            ResourceType::Wood => Color::BROWN,
            ResourceType::Stone => Color::GRAY,
            ResourceType::IronOre => Color::new(255, 165, 0, 255),
            ResourceType::Coal => Color::new(64, 64, 64, 255),
            ResourceType::Clay => Color::new(139, 69, 19, 255),
            ResourceType::CopperOre => Color::new(184, 115, 51, 255),
            ResourceType::Cotton => Color::new(255, 255, 240, 255),
            ResourceType::Water => Color::new(100, 149, 237, 255),
            ResourceType::WheatSeeds => Color::new(255, 215, 0, 255),
            ResourceType::Wheat => Color::new(255, 215, 0, 255),
            
            // Animal resources
            ResourceType::Egg => Color::new(255, 248, 220, 255),
            ResourceType::RawChicken => Color::new(255, 192, 203, 255),
            ResourceType::CookedChicken => Color::new(160, 82, 45, 255),
            ResourceType::ChickenFeathers => Color::new(245, 245, 220, 255),
            
            // Wood products
            ResourceType::WoodenPlanks => Color::new(160, 82, 45, 255),
            ResourceType::WoodenBeams => Color::new(139, 69, 19, 255),
            ResourceType::WoodenGears => Color::new(210, 180, 140, 255),
            ResourceType::WoodenFrames => Color::new(205, 133, 63, 255),
            ResourceType::WoodenRollers => Color::new(222, 184, 135, 255),
            
            // Metal products
            ResourceType::Charcoal => Color::new(36, 36, 36, 255),
            ResourceType::IronBloom => Color::new(160, 160, 160, 255),
            ResourceType::WroughtIron => Color::new(128, 128, 128, 255),
            ResourceType::IronPlates => Color::new(192, 192, 192, 255),
            ResourceType::IronGears => Color::new(169, 169, 169, 255),
            ResourceType::MetalRods => Color::new(105, 105, 105, 255),
            ResourceType::SteelIngots => Color::new(70, 130, 180, 255),
            ResourceType::SteelPlates => Color::new(100, 149, 237, 255),
            
            // Copper products
            ResourceType::CopperIngots => Color::new(184, 115, 51, 255),
            ResourceType::CopperPlates => Color::new(205, 127, 50, 255),
            ResourceType::CopperWire => Color::new(255, 140, 0, 255),
            ResourceType::CopperCoils => Color::new(255, 165, 0, 255),
            ResourceType::CopperPipes => Color::new(218, 165, 32, 255),
            ResourceType::BronzeAlloy => Color::new(205, 127, 50, 255),
            ResourceType::ElectricalComponents => Color::new(255, 215, 0, 255),
            
            // Textiles
            ResourceType::Threads => Color::new(245, 245, 220, 255),
            ResourceType::Fabric => Color::new(230, 230, 250, 255),
            ResourceType::ClothStrips => Color::new(255, 228, 196, 255),
            ResourceType::ReinforcedFabric => Color::new(188, 143, 143, 255),
            
            // Food
            ResourceType::Flour => Color::new(255, 250, 240, 255),
            ResourceType::Dough => Color::new(255, 228, 181, 255),
            ResourceType::Bread => Color::new(210, 180, 140, 255),
            
            // Steam
            ResourceType::WaterBucket => Color::new(100, 149, 237, 255),
            ResourceType::SteamPipes => Color::new(128, 128, 128, 255),
            ResourceType::PressureValve => Color::new(255, 215, 0, 255),
            
            // Tools
            ResourceType::Scythe => Color::new(139, 69, 19, 255),
            
            // Automation
            ResourceType::BasicConveyorBelt => Color::BROWN,
            ResourceType::ReinforcedConveyor => Color::new(128, 128, 128, 255),
            ResourceType::SteamConveyor => Color::new(169, 169, 169, 255),
            ResourceType::ElectricConveyor => Color::new(255, 215, 0, 255),
            ResourceType::PowerCables => Color::new(255, 140, 0, 255),
            
            // Buildings
            ResourceType::CharcoalPit => Color::new(101, 67, 33, 255),
            ResourceType::CrudeFurnace => Color::new(139, 69, 19, 255),
            ResourceType::BloomeryFurnace => Color::new(139, 69, 19, 255),
            ResourceType::StoneAnvil => Color::GRAY,
            ResourceType::SpinningWheel => Color::BROWN,
            ResourceType::WeavingMachine => Color::new(160, 82, 45, 255),
            ResourceType::AdvancedForge => Color::new(105, 105, 105, 255),
            ResourceType::WheatFarm => Color::new(34, 139, 34, 255),
            ResourceType::Windmill => Color::new(205, 133, 63, 255),
            ResourceType::WaterMill => Color::new(139, 69, 19, 255),
            ResourceType::StoneOven => Color::GRAY,
            ResourceType::GrainSilo => Color::new(205, 133, 63, 255),
            ResourceType::SteamBoiler => Color::new(128, 128, 128, 255),
            ResourceType::SteamDistributionHub => Color::new(169, 169, 169, 255),
            ResourceType::WaterPump => Color::new(100, 149, 237, 255),
            ResourceType::SteamPump => Color::new(128, 128, 128, 255),
            ResourceType::SteamHammer => Color::new(105, 105, 105, 255),
            ResourceType::SortingMachine => Color::new(192, 192, 192, 255),
            ResourceType::SteamEngine => Color::new(70, 130, 180, 255),
            ResourceType::ConveyorBelt => Color::BROWN,
        }
    }
    
    pub fn get_icon_filename(&self) -> &'static str {
        match self {
            // Basic resources
            ResourceType::Wood => "icons/wood.png",
            ResourceType::Stone => "icons/stone.png",
            ResourceType::IronOre => "icons/iron_ore.png",
            ResourceType::Coal => "icons/coal.png",
            ResourceType::Clay => "icons/clay.png",
            ResourceType::CopperOre => "icons/copper_ore.png",
            ResourceType::Cotton => "icons/cotton.png",
            ResourceType::Water => "icons/water.png",
            ResourceType::WheatSeeds => "icons/wheat_seeds.png",
            ResourceType::Wheat => "icons/wheat.png",
            
            // Animal resources
            ResourceType::Egg => "icons/egg.png",
            ResourceType::RawChicken => "icons/raw_chicken.png",
            ResourceType::CookedChicken => "icons/cooked_chicken.png",
            ResourceType::ChickenFeathers => "icons/chicken_feathers.png",
            
            // Wood products
            ResourceType::WoodenPlanks => "icons/wooden_planks.png",
            ResourceType::WoodenBeams => "icons/wooden_beams.png",
            ResourceType::WoodenGears => "icons/wooden_gears.png",
            ResourceType::WoodenFrames => "icons/wooden_frames.png",
            ResourceType::WoodenRollers => "icons/wooden_rollers.png",
            
            // Metal products
            ResourceType::Charcoal => "icons/charcoal.png",
            ResourceType::IronBloom => "icons/iron_bloom.png",
            ResourceType::WroughtIron => "icons/wrought_iron.png",
            ResourceType::IronPlates => "icons/iron_plates.png",
            ResourceType::IronGears => "icons/iron_gears.png",
            ResourceType::MetalRods => "icons/metal_rods.png",
            ResourceType::SteelIngots => "icons/steel_ingots.png",
            ResourceType::SteelPlates => "icons/steel_plates.png",
            
            // Copper products
            ResourceType::CopperIngots => "icons/copper_ingots.png",
            ResourceType::CopperPlates => "icons/copper_plates.png",
            ResourceType::CopperWire => "icons/copper_wire.png",
            ResourceType::CopperCoils => "icons/copper_coils.png",
            ResourceType::CopperPipes => "icons/copper_pipes.png",
            ResourceType::BronzeAlloy => "icons/bronze_alloy.png",
            ResourceType::ElectricalComponents => "icons/electrical_components.png",
            
            // Textiles
            ResourceType::Threads => "icons/threads.png",
            ResourceType::Fabric => "icons/fabric.png",
            ResourceType::ClothStrips => "icons/cloth_strips.png",
            ResourceType::ReinforcedFabric => "icons/reinforced_fabric.png",
            
            // Food
            ResourceType::Flour => "icons/flour.png",
            ResourceType::Dough => "icons/dough.png",
            ResourceType::Bread => "icons/bread.png",
            
            // Steam
            ResourceType::WaterBucket => "icons/water_bucket.png",
            ResourceType::SteamPipes => "icons/steam_pipes.png",
            ResourceType::PressureValve => "icons/pressure_valve.png",
            
            // Tools
            ResourceType::Scythe => "icons/scythe.png",
            
            // Automation
            ResourceType::BasicConveyorBelt => "icons/basic_conveyor_belt.png",
            ResourceType::ReinforcedConveyor => "icons/reinforced_conveyor.png",
            ResourceType::SteamConveyor => "icons/steam_conveyor.png",
            ResourceType::ElectricConveyor => "icons/electric_conveyor.png",
            ResourceType::PowerCables => "icons/power_cables.png",
            
            // Buildings
            ResourceType::CharcoalPit => "icons/charcoal_pit.png",
            ResourceType::CrudeFurnace => "icons/crude_furnace.png",
            ResourceType::BloomeryFurnace => "icons/bloomery_furnace.png",
            ResourceType::StoneAnvil => "icons/stone_anvil.png",
            ResourceType::SpinningWheel => "icons/spinning_wheel.png",
            ResourceType::WeavingMachine => "icons/weaving_machine.png",
            ResourceType::AdvancedForge => "icons/advanced_forge.png",
            ResourceType::WheatFarm => "icons/wheat_farm.png",
            ResourceType::Windmill => "icons/windmill.png",
            ResourceType::WaterMill => "icons/water_mill.png",
            ResourceType::StoneOven => "icons/stone_oven.png",
            ResourceType::GrainSilo => "icons/grain_silo.png",
            ResourceType::SteamBoiler => "icons/steam_boiler.png",
            ResourceType::SteamDistributionHub => "icons/steam_distribution_hub.png",
            ResourceType::WaterPump => "icons/water_pump.png",
            ResourceType::SteamPump => "icons/steam_pump.png",
            ResourceType::SteamHammer => "icons/steam_hammer.png",
            ResourceType::SortingMachine => "icons/sorting_machine.png",
            ResourceType::SteamEngine => "icons/steam_engine.png",
            ResourceType::ConveyorBelt => "icons/conveyor_belt.png",
        }
    }
    
    pub fn get_max_stack_size(&self) -> u32 {
        match self {
            // Basic resources - high stack size
            ResourceType::Wood | ResourceType::Stone | ResourceType::IronOre | 
            ResourceType::Coal | ResourceType::Clay | ResourceType::CopperOre | 
            ResourceType::Cotton | ResourceType::Water | ResourceType::WheatSeeds | 
            ResourceType::Wheat | ResourceType::Egg | ResourceType::RawChicken | 
            ResourceType::CookedChicken | ResourceType::ChickenFeathers => 200,
            
            // Processed materials - medium stack size
            ResourceType::WoodenPlanks | ResourceType::WoodenBeams | ResourceType::WoodenGears |
            ResourceType::WoodenFrames | ResourceType::WoodenRollers | ResourceType::Charcoal | 
            ResourceType::IronBloom | ResourceType::WroughtIron | ResourceType::IronPlates | 
            ResourceType::IronGears | ResourceType::MetalRods | ResourceType::SteelIngots | 
            ResourceType::SteelPlates | ResourceType::CopperIngots | ResourceType::CopperPlates | 
            ResourceType::CopperWire | ResourceType::CopperCoils | ResourceType::CopperPipes | 
            ResourceType::BronzeAlloy | ResourceType::ElectricalComponents | ResourceType::Threads | 
            ResourceType::Fabric | ResourceType::ClothStrips | ResourceType::ReinforcedFabric |
            ResourceType::Flour | ResourceType::Dough | ResourceType::Bread | ResourceType::WaterBucket |
            ResourceType::SteamPipes | ResourceType::PressureValve | ResourceType::PowerCables => 100,
            
            // Tools - low stack size
            ResourceType::Scythe => 10,
            
            // Automation - medium stack size
            ResourceType::BasicConveyorBelt | ResourceType::ReinforcedConveyor | 
            ResourceType::SteamConveyor | ResourceType::ElectricConveyor | ResourceType::ConveyorBelt => 50,
            
            // Small buildings - low stack size
            ResourceType::CharcoalPit | ResourceType::CrudeFurnace | ResourceType::StoneAnvil | 
            ResourceType::SpinningWheel | ResourceType::WaterPump => 5,
            
            // Medium buildings - very low stack size
            ResourceType::BloomeryFurnace | ResourceType::WeavingMachine | ResourceType::AdvancedForge |
            ResourceType::StoneOven | ResourceType::SteamBoiler | ResourceType::SteamDistributionHub |
            ResourceType::SteamPump | ResourceType::SteamHammer | ResourceType::SortingMachine => 3,
            
            // Large buildings - very low stack size
            ResourceType::WheatFarm | ResourceType::Windmill | ResourceType::WaterMill | 
            ResourceType::GrainSilo | ResourceType::SteamEngine => 2,
        }
    }
    
    pub fn is_building(&self) -> bool {
        matches!(self, 
            ResourceType::CharcoalPit | ResourceType::CrudeFurnace | ResourceType::BloomeryFurnace | 
            ResourceType::StoneAnvil | ResourceType::SpinningWheel | ResourceType::WeavingMachine | 
            ResourceType::AdvancedForge | ResourceType::WheatFarm | ResourceType::Windmill | 
            ResourceType::WaterMill | ResourceType::StoneOven | ResourceType::GrainSilo | 
            ResourceType::SteamBoiler | ResourceType::SteamDistributionHub | ResourceType::WaterPump | 
            ResourceType::SteamPump | ResourceType::SteamHammer | ResourceType::SortingMachine | 
            ResourceType::SteamEngine | ResourceType::ConveyorBelt | ResourceType::BasicConveyorBelt |
            ResourceType::ReinforcedConveyor | ResourceType::SteamConveyor | ResourceType::ElectricConveyor
        )
    }
}

#[derive(Clone, Debug)]
pub struct InventorySlot {
    pub resource_type: Option<ResourceType>,
    pub amount: u32,
}

impl InventorySlot {
    pub fn new() -> Self {
        Self {
            resource_type: None,
            amount: 0,
        }
    }
    
    pub fn with_resource(resource_type: ResourceType, amount: u32) -> Self {
        Self {
            resource_type: Some(resource_type),
            amount,
        }
    }
    
    pub fn is_empty(&self) -> bool {
        self.resource_type.is_none() || self.amount == 0
    }
    pub fn can_add(&self, resource_type: ResourceType, amount: u32) -> bool {
        if self.is_empty() {
            true
        } else if let Some(existing_type) = self.resource_type {
            existing_type == resource_type && 
            self.amount + amount <= resource_type.get_max_stack_size()
        } else {
            false
        }
    }
    
    pub fn add(&mut self, resource_type: ResourceType, amount: u32) -> u32 {
        let max_stack = resource_type.get_max_stack_size();
        
        if self.is_empty() {
            let amount_to_add = amount.min(max_stack);
            self.resource_type = Some(resource_type);
            self.amount = amount_to_add;
            amount_to_add
        } else if let Some(existing_type) = self.resource_type {
            if existing_type == resource_type {
                let available_space = max_stack.saturating_sub(self.amount);
                let amount_to_add = amount.min(available_space);
                self.amount += amount_to_add;
                amount_to_add
            } else {
                0
            }
        } else {
            0
        }
    }
    
    pub fn remove(&mut self, amount: u32) -> u32 {
        let removed = amount.min(self.amount);
        self.amount -= removed;
        if self.amount == 0 {
            self.resource_type = None;
        }
        removed
    }
}

#[derive(Clone, Debug)]
pub struct Inventory {
    pub slots: Vec<InventorySlot>,
    pub slot_count: usize,
}

impl Inventory {
    pub fn new() -> Self {
        let slot_count = 100; // Expanded from 48 to 100 for future expansion
        let mut slots = vec![InventorySlot::new(); slot_count];
        
        // Add starting resources to first few slots for testing
        slots[0] = InventorySlot::with_resource(ResourceType::Wood, 50);
        slots[1] = InventorySlot::with_resource(ResourceType::Stone, 100);
        slots[2] = InventorySlot::with_resource(ResourceType::Clay, 25);
        slots[3] = InventorySlot::with_resource(ResourceType::IronOre, 30);
        slots[4] = InventorySlot::with_resource(ResourceType::Coal, 20);
        slots[5] = InventorySlot::with_resource(ResourceType::CopperOre, 25);
        slots[6] = InventorySlot::with_resource(ResourceType::Cotton, 15);
        slots[7] = InventorySlot::with_resource(ResourceType::Water, 40);
        slots[8] = InventorySlot::with_resource(ResourceType::WheatSeeds, 10);
        slots[9] = InventorySlot::with_resource(ResourceType::Wheat, 20); 
        
        Self { 
            slots,
            slot_count,
        }
    }
    
    pub fn get_amount(&self, resource: &ResourceType) -> u32 {
        self.slots.iter()
            .filter_map(|slot| {
                if let Some(slot_resource) = slot.resource_type {
                    if slot_resource == *resource {
                        Some(slot.amount)
                    } else {
                        None
                    }
                } else {
                    None
                }
            })
            .sum()
    }
    
    pub fn add_resource(&mut self, resource: ResourceType, amount: u32) -> u32 {
        let mut remaining = amount;
        
        // First, try to add to existing stacks
        for slot in &mut self.slots {
            if let Some(slot_resource) = slot.resource_type {
                if slot_resource == resource {
                    let added = slot.add(resource, remaining);
                    remaining -= added;
                    if remaining == 0 {
                        return amount;
                    }
                }
            }
        }
        
        // Then, try to add to empty slots
        for slot in &mut self.slots {
            if slot.is_empty() {
                let added = slot.add(resource, remaining);
                remaining -= added;
                if remaining == 0 {
                    return amount;
                }
            }
        }
        
        amount - remaining // Return how much was actually added
    }
    
    pub fn remove_resource(&mut self, resource: ResourceType, amount: u32) -> bool {
        let available = self.get_amount(&resource);
        if available < amount {
            return false;
        }
        
        let mut remaining = amount;
        for slot in &mut self.slots {
            if let Some(slot_resource) = slot.resource_type {
                if slot_resource == resource {
                    let removed = slot.remove(remaining);
                    remaining -= removed;
                    if remaining == 0 {
                        break;
                    }
                }
            }
        }
        
        true
    }
    
    pub fn get_slot(&self, index: usize) -> Option<&InventorySlot> {
        self.slots.get(index)
    }
    
    pub fn get_slot_mut(&mut self, index: usize) -> Option<&mut InventorySlot> {
        self.slots.get_mut(index)
    }
    
    pub fn clear_slot(&mut self, index: usize) {
        if let Some(slot) = self.slots.get_mut(index) {
            *slot = InventorySlot::new();
        }
    }
    
    pub fn add_to_slot(&mut self, index: usize, resource_type: ResourceType, amount: u32) -> bool {
        if let Some(slot) = self.slots.get_mut(index) {
            if slot.is_empty() {
                *slot = InventorySlot::with_resource(resource_type, amount);
                true
            } else if slot.resource_type == Some(resource_type) {
                slot.add(resource_type, amount);
                true
            } else {
                false
            }
        } else {
            false
        }
    }
}
