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
        
    // Processed materials
    Charcoal,
    IronBloom,
    WroughtIron,
    IronPlates,
    IronGears,
    MetalRods,
    Threads,
    Fabric,
    ClothStrips,

    CharcoalPit,
    BloomeryFurnace,
    StoneAnvil,
    SpinningWheel,
    WeavingMachine,
    ConveyorBelt,
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
            ResourceType::CharcoalPit => "Charcoal Pit",
            ResourceType::BloomeryFurnace => "Bloomery Furnace",
            ResourceType::StoneAnvil => "Stone Anvil",
            ResourceType::SpinningWheel => "Spinning Wheel",
            ResourceType::WeavingMachine => "Weaving Machine",
            ResourceType::ConveyorBelt => "Conveyor Belt",
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
            ResourceType::ClothStrips => Color::new(255, 228, 196, 255),
            ResourceType::CharcoalPit => Color::new(101, 67, 33, 255),
            ResourceType::BloomeryFurnace => Color::new(139, 69, 19, 255),
            ResourceType::StoneAnvil => Color::GRAY,
            ResourceType::SpinningWheel => Color::BROWN,
            ResourceType::WeavingMachine => Color::new(160, 82, 45, 255),
            ResourceType::ConveyorBelt => Color::BROWN,
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
            ResourceType::CharcoalPit => "icons/charcoal_pit.png",
            ResourceType::BloomeryFurnace => "icons/bloomery_furnace.png", 
            ResourceType::StoneAnvil => "icons/stone_anvil.png",
            ResourceType::SpinningWheel => "icons/spinning_wheel.png",
            ResourceType::WeavingMachine => "icons/weaving_machine.png",
            ResourceType::ConveyorBelt => "icons/conveyor_belt.png",
        }
    }
    
    pub fn get_max_stack_size(&self) -> u32 {
        match self {
            // Basic resources - high stack size
            ResourceType::Wood | ResourceType::Stone | ResourceType::IronOre | 
            ResourceType::Coal | ResourceType::Clay | ResourceType::CopperOre | 
            ResourceType::Cotton => 200,
            
            // Processed materials - medium stack size
            ResourceType::Charcoal | ResourceType::IronBloom | ResourceType::WroughtIron | 
            ResourceType::IronPlates | ResourceType::IronGears | ResourceType::MetalRods |
            ResourceType::Threads | ResourceType::Fabric | ResourceType::ClothStrips => 100,
            
            // Buildings - low stack size
            ResourceType::CharcoalPit | ResourceType::StoneAnvil | 
            ResourceType::SpinningWheel => 5,
            
            ResourceType::BloomeryFurnace | ResourceType::WeavingMachine => 3,
            
            ResourceType::ConveyorBelt => 50,
        }
    }
    
    pub fn is_building(&self) -> bool {
        matches!(self, 
            ResourceType::CharcoalPit | ResourceType::BloomeryFurnace | 
            ResourceType::StoneAnvil | ResourceType::SpinningWheel | 
            ResourceType::WeavingMachine | ResourceType::ConveyorBelt
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
        let slot_count = 48; // 8x6 grid
        let mut slots = vec![InventorySlot::new(); slot_count];
        
        // Add starting resources to first few slots
        slots[0] = InventorySlot::with_resource(ResourceType::Wood, 20);
        slots[1] = InventorySlot::with_resource(ResourceType::Stone, 30);
        slots[2] = InventorySlot::with_resource(ResourceType::Clay, 5); 
        
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
