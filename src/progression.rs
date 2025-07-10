use crate::types::*;
use crate::crafting::CraftableItem;
use raylib::prelude::*;
use std::collections::HashMap;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Tier {
    StoneAge = 1,
    EarlyCivilization = 2,
    BronzeAge = 3,
    IronAge = 4,
    AgriculturalAge = 5,
    SteamAge = 6,
    IndustrialAge = 7,
    ElectricAge = 8,
}

#[derive(Clone, Debug)]
pub struct Task {
    pub id: String,
    pub description: String,
    pub task_type: TaskType,
    pub target: u32,
    pub current: u32,
    pub completed: bool,
}

#[derive(Clone, Debug)]
pub enum TaskType {
    CollectResource(ResourceType),
    CraftItem(CraftableItem),
    BuildStructure(BuildingType),
    ProcessItems(ResourceType, u32), // Process X amount of items
    ConnectBuildings(u32), // Connect X buildings with steam/power
    AchieveProductionRate(u32), // Items per hour
}

#[derive(Clone, Debug)]
pub struct TierRequirements {
    pub tier: Tier,
    pub name: &'static str,
    pub description: &'static str,
    pub tasks: Vec<Task>,
    pub unlocked_recipes: Vec<CraftableItem>,
}

pub struct ProgressionSystem {
    pub current_tier: Tier,
    pub tiers: HashMap<Tier, TierRequirements>,
    pub completed_tiers: Vec<Tier>,
    pub task_progress: HashMap<String, u32>,
}

impl Tier {
    pub fn get_name(&self) -> &'static str {
        match self {
            Tier::StoneAge => "Stone Age",
            Tier::EarlyCivilization => "Early Civilization", 
            Tier::BronzeAge => "Bronze Age",
            Tier::IronAge => "Iron Age",
            Tier::AgriculturalAge => "Agricultural Age",
            Tier::SteamAge => "Steam Age",
            Tier::IndustrialAge => "Industrial Age",
            Tier::ElectricAge => "Electric Age",
        }
    }

    pub fn get_color(&self) -> Color {
        match self {
            Tier::StoneAge => Color::new(139, 69, 19, 255), // Brown
            Tier::EarlyCivilization => Color::new(160, 82, 45, 255), // Saddle Brown
            Tier::BronzeAge => Color::new(205, 127, 50, 255), // Peru
            Tier::IronAge => Color::LIGHTGRAY,
            Tier::AgriculturalAge => Color::new(34, 139, 34, 255), // Forest Green
            Tier::SteamAge => Color::new(128, 128, 128, 255), // Gray
            Tier::IndustrialAge => Color::new(70, 130, 180, 255), // Steel Blue
            Tier::ElectricAge => Color::new(255, 215, 0, 255), // Gold
        }
    }

    pub fn next(&self) -> Option<Tier> {
        match self {
            Tier::StoneAge => Some(Tier::EarlyCivilization),
            Tier::EarlyCivilization => Some(Tier::BronzeAge),
            Tier::BronzeAge => Some(Tier::IronAge),
            Tier::IronAge => Some(Tier::AgriculturalAge),
            Tier::AgriculturalAge => Some(Tier::SteamAge),
            Tier::SteamAge => Some(Tier::IndustrialAge),
            Tier::IndustrialAge => Some(Tier::ElectricAge),
            Tier::ElectricAge => None,
        }
    }
}

impl Task {
    pub fn new(id: &str, description: &str, task_type: TaskType, target: u32) -> Self {
        Self {
            id: id.to_string(),
            description: description.to_string(),
            task_type,
            target,
            current: 0,
            completed: false,
        }
    }

    pub fn update_progress(&mut self, amount: u32) {
        if !self.completed {
            self.current = (self.current + amount).min(self.target);
            if self.current >= self.target {
                self.completed = true;
            }
        }
    }

    pub fn get_progress_text(&self) -> String {
        if self.completed {
            format!("✓ {}", self.description)
        } else {
            format!("{}: {}/{}", self.description, self.current, self.target)
        }
    }
}

impl ProgressionSystem {
    pub fn new() -> Self {
        let mut system = Self {
            current_tier: Tier::StoneAge,
            tiers: HashMap::new(),
            completed_tiers: Vec::new(),
            task_progress: HashMap::new(),
        };
        
        system.initialize_tiers();
        system
    }

    fn initialize_tiers(&mut self) {
        // Tier 1: Stone Age
        let stone_age_tasks = vec![
            Task::new("collect_rocks", "Collect Rocks", TaskType::CollectResource(ResourceType::Rocks), 20),
            Task::new("collect_sticks", "Collect Sticks", TaskType::CollectResource(ResourceType::Sticks), 15),
            Task::new("collect_twigs", "Collect Twigs", TaskType::CollectResource(ResourceType::Twigs), 10),
            Task::new("craft_stone_pickaxe", "Craft Stone Pickaxe", TaskType::CraftItem(CraftableItem::StonePickaxe), 1),
            Task::new("craft_stone_axe", "Craft Stone Axe", TaskType::CraftItem(CraftableItem::StoneAxe), 1),
            Task::new("build_campfire", "Build Campfire", TaskType::BuildStructure(BuildingType::Campfire), 1),
            Task::new("make_charcoal", "Make Charcoal", TaskType::CollectResource(ResourceType::Charcoal), 5),
        ];

        let stone_age_unlocks = vec![
            CraftableItem::StonePickaxe,
            CraftableItem::StoneAxe,
            CraftableItem::StoneShovel,
            CraftableItem::StoneSword,
            CraftableItem::Charcoal,
            CraftableItem::WoodenPlanks,
            CraftableItem::WoodenBeams,
            CraftableItem::WoodenGears,
            CraftableItem::WoodenFrames,
            CraftableItem::Campfire,
            CraftableItem::CharcoalPit,
            CraftableItem::CrudeFurnace,
        ];

        self.tiers.insert(Tier::StoneAge, TierRequirements {
            tier: Tier::StoneAge,
            name: "Stone Age",
            description: "Learn the basics of survival and tool-making",
            tasks: stone_age_tasks,
            unlocked_recipes: stone_age_unlocks,
        });

        // Tier 2: Early Civilization
        let early_civ_tasks = vec![
            Task::new("build_stone_anvil", "Build Stone Anvil", TaskType::BuildStructure(BuildingType::StoneAnvil), 1),
            Task::new("collect_clay", "Collect Clay", TaskType::CollectResource(ResourceType::Clay), 10),
            Task::new("collect_plant_fiber", "Collect Plant Fiber", TaskType::CollectResource(ResourceType::PlantFiber), 15),
            Task::new("smelt_copper", "Smelt Copper Ingots", TaskType::CraftItem(CraftableItem::CopperIngots), 5),
            Task::new("create_copper_wire", "Create Copper Wire", TaskType::CraftItem(CraftableItem::CopperWire), 10),
            Task::new("cook_chicken", "Cook Chicken", TaskType::CraftItem(CraftableItem::CookedChicken), 3),
            Task::new("spin_threads", "Spin Threads", TaskType::CraftItem(CraftableItem::Threads), 5),
        ];

        let early_civ_unlocks = vec![
            CraftableItem::StoneAnvil,
            CraftableItem::CopperIngots,
            CraftableItem::CopperPlates,
            CraftableItem::CopperWire,
            CraftableItem::CopperCoils,
            CraftableItem::CopperPipes,
            CraftableItem::CookedChicken,
            CraftableItem::Threads,
        ];

        self.tiers.insert(Tier::EarlyCivilization, TierRequirements {
            tier: Tier::EarlyCivilization,
            name: "Early Civilization",
            description: "Develop basic metallurgy and food preparation",
            tasks: early_civ_tasks,
            unlocked_recipes: early_civ_unlocks,
        });

        // Tier 3: Bronze Age
        let bronze_age_tasks = vec![
            Task::new("create_bronze", "Create Bronze Alloy", TaskType::CraftItem(CraftableItem::BronzeAlloy), 5),
            Task::new("build_spinning_wheel", "Build Spinning Wheel", TaskType::BuildStructure(BuildingType::SpinningWheel), 1),
            Task::new("build_weaving_machine", "Build Weaving Machine", TaskType::BuildStructure(BuildingType::WeavingMachine), 1),
            Task::new("craft_fabric", "Craft Fabric", TaskType::CraftItem(CraftableItem::Fabric), 10),
            Task::new("create_cloth_strips", "Create Cloth Strips", TaskType::CraftItem(CraftableItem::ClothStrips), 5),
            Task::new("collect_cotton", "Collect Cotton", TaskType::CollectResource(ResourceType::Cotton), 20),
            Task::new("make_reinforced_fabric", "Make Reinforced Fabric", TaskType::CraftItem(CraftableItem::ReinforcedFabric), 3),
        ];

        let bronze_age_unlocks = vec![
            CraftableItem::BronzeAlloy,
            CraftableItem::SpinningWheel,
            CraftableItem::WeavingMachine,
            CraftableItem::Fabric,
            CraftableItem::ClothStrips,
            CraftableItem::ReinforcedFabric,
            CraftableItem::ElectricalComponents,
        ];

        self.tiers.insert(Tier::BronzeAge, TierRequirements {
            tier: Tier::BronzeAge,
            name: "Bronze Age",
            description: "Master advanced metallurgy and textile production",
            tasks: bronze_age_tasks,
            unlocked_recipes: bronze_age_unlocks,
        });

        // Tier 4: Iron Age
        let iron_age_tasks = vec![
            Task::new("build_bloomery", "Build Bloomery Furnace", TaskType::BuildStructure(BuildingType::BloomeryFurnace), 1),
            Task::new("smelt_iron_bloom", "Smelt Iron Bloom", TaskType::CraftItem(CraftableItem::IronBloom), 10),
            Task::new("forge_wrought_iron", "Forge Wrought Iron", TaskType::CraftItem(CraftableItem::WroughtIron), 15),
            Task::new("create_iron_plates", "Create Iron Plates", TaskType::CraftItem(CraftableItem::IronPlates), 20),
            Task::new("craft_iron_pickaxe", "Craft Iron Pickaxe", TaskType::CraftItem(CraftableItem::IronPickaxe), 1),
            Task::new("craft_iron_axe", "Craft Iron Axe", TaskType::CraftItem(CraftableItem::IronAxe), 1),
            Task::new("build_advanced_forge", "Build Advanced Forge", TaskType::BuildStructure(BuildingType::AdvancedForge), 1),
            Task::new("create_steel", "Create Steel Ingots", TaskType::CraftItem(CraftableItem::SteelIngots), 10),
        ];

        let iron_age_unlocks = vec![
            CraftableItem::IronPickaxe,
            CraftableItem::IronAxe,
            CraftableItem::IronShovel,
            CraftableItem::IronSword,
            CraftableItem::BloomeryFurnace,
            CraftableItem::AdvancedForge,
            CraftableItem::IronBloom,
            CraftableItem::WroughtIron,
            CraftableItem::IronPlates,
            CraftableItem::IronGears,
            CraftableItem::MetalRods,
            CraftableItem::SteelIngots,
            CraftableItem::SteelPlates,
        ];

        self.tiers.insert(Tier::IronAge, TierRequirements {
            tier: Tier::IronAge,
            name: "Iron Age",
            description: "Unlock advanced tools and iron processing",
            tasks: iron_age_tasks,
            unlocked_recipes: iron_age_unlocks,
        });

        // Tier 5: Agricultural Age
        let agricultural_tasks = vec![
            Task::new("build_wheat_farm", "Build Wheat Farm", TaskType::BuildStructure(BuildingType::WheatFarm), 1),
            Task::new("harvest_wheat", "Harvest Wheat", TaskType::CollectResource(ResourceType::Wheat), 50),
            Task::new("build_windmill", "Build Windmill", TaskType::BuildStructure(BuildingType::Windmill), 1),
            Task::new("create_flour", "Create Flour", TaskType::CraftItem(CraftableItem::Flour), 20),
            Task::new("build_stone_oven", "Build Stone Oven", TaskType::BuildStructure(BuildingType::StoneOven), 1),
            Task::new("bake_bread", "Bake Bread", TaskType::CraftItem(CraftableItem::Bread), 10),
            Task::new("craft_scythe", "Craft Scythe", TaskType::CraftItem(CraftableItem::Scythe), 1),
            Task::new("build_grain_silo", "Build Grain Silo", TaskType::BuildStructure(BuildingType::GrainSilo), 1),
        ];

        let agricultural_unlocks = vec![
            CraftableItem::WheatFarm,
            CraftableItem::Windmill,
            CraftableItem::StoneOven,
            CraftableItem::GrainSilo,
            CraftableItem::Flour,
            CraftableItem::Dough,
            CraftableItem::Bread,
            CraftableItem::Scythe,
        ];

        self.tiers.insert(Tier::AgriculturalAge, TierRequirements {
            tier: Tier::AgriculturalAge,
            name: "Agricultural Age",
            description: "Develop farming and food processing systems",
            tasks: agricultural_tasks,
            unlocked_recipes: agricultural_unlocks,
        });

        // Tier 6: Steam Age
        let steam_age_tasks = vec![
            Task::new("build_water_pump", "Build Water Pump", TaskType::BuildStructure(BuildingType::WaterPump), 1),
            Task::new("create_water_buckets", "Create Water Buckets", TaskType::CraftItem(CraftableItem::WaterBucket), 10),
            Task::new("craft_steam_pipes", "Craft Steam Pipes", TaskType::CraftItem(CraftableItem::SteamPipes), 20),
            Task::new("build_steam_boiler", "Build Steam Boiler", TaskType::BuildStructure(BuildingType::SteamBoiler), 1),
            Task::new("create_pressure_valves", "Create Pressure Valves", TaskType::CraftItem(CraftableItem::PressureValve), 5),
            Task::new("build_steam_hub", "Build Steam Distribution Hub", TaskType::BuildStructure(BuildingType::SteamDistributionHub), 1),
            Task::new("connect_steam_buildings", "Connect Buildings with Steam", TaskType::ConnectBuildings(3), 3),
        ];

        let steam_age_unlocks = vec![
            CraftableItem::WaterPump,
            CraftableItem::WaterBucket,
            CraftableItem::SteamPipes,
            CraftableItem::PressureValve,
            CraftableItem::SteamBoiler,
            CraftableItem::SteamDistributionHub,
        ];

        self.tiers.insert(Tier::SteamAge, TierRequirements {
            tier: Tier::SteamAge,
            name: "Steam Age",
            description: "Harness the power of steam for industry",
            tasks: steam_age_tasks,
            unlocked_recipes: steam_age_unlocks,
        });

        // Tier 7: Industrial Age
        let industrial_tasks = vec![
            Task::new("build_conveyors", "Build Basic Conveyor Belts", TaskType::CraftItem(CraftableItem::BasicConveyorBelt), 10),
            Task::new("create_steam_engine", "Create Steam Engine", TaskType::CraftItem(CraftableItem::SteamEngine), 1),
            Task::new("build_steam_hammer", "Build Steam Hammer", TaskType::BuildStructure(BuildingType::SteamHammer), 1),
            Task::new("create_electrical", "Create Electrical Components", TaskType::CraftItem(CraftableItem::ElectricalComponents), 15),
            Task::new("build_reinforced_conveyors", "Build Reinforced Conveyors", TaskType::CraftItem(CraftableItem::ReinforcedConveyor), 5),
            Task::new("setup_automation", "Set up Production Line", TaskType::ConnectBuildings(5), 5),
            Task::new("process_items", "Process Items Through Automation", TaskType::ProcessItems(ResourceType::IronPlates, 100), 100),
        ];

        let industrial_unlocks = vec![
            CraftableItem::BasicConveyorBelt,
            CraftableItem::ReinforcedConveyor,
            CraftableItem::SteamPump,
            CraftableItem::SteamHammer,
            CraftableItem::SteamEngine,
            CraftableItem::ConveyorBelt,
        ];

        self.tiers.insert(Tier::IndustrialAge, TierRequirements {
            tier: Tier::IndustrialAge,
            name: "Industrial Age",
            description: "Develop automation and mass production",
            tasks: industrial_tasks,
            unlocked_recipes: industrial_unlocks,
        });

        // Tier 8: Electric Age
        let electric_tasks = vec![
            Task::new("create_power_cables", "Create Power Cables", TaskType::CraftItem(CraftableItem::PowerCables), 50),
            Task::new("build_electric_conveyors", "Build Electric Conveyors", TaskType::CraftItem(CraftableItem::ElectricConveyor), 3),
            Task::new("build_sorting_machines", "Build Sorting Machines", TaskType::CraftItem(CraftableItem::SortingMachine), 2),
            Task::new("create_power_network", "Create Electrical Network", TaskType::ConnectBuildings(10), 10),
            Task::new("sort_items", "Sort Items Automatically", TaskType::ProcessItems(ResourceType::IronPlates, 500), 500),
            Task::new("automate_factory", "Establish Automated Factory", TaskType::ConnectBuildings(15), 15),
            Task::new("production_rate", "Achieve Production Rate", TaskType::AchieveProductionRate(1000), 1000),
        ];

        let electric_unlocks = vec![
            CraftableItem::ElectricConveyor,
            CraftableItem::SortingMachine,
            CraftableItem::PowerCables,
            CraftableItem::SteamConveyor,
        ];

        self.tiers.insert(Tier::ElectricAge, TierRequirements {
            tier: Tier::ElectricAge,
            name: "Electric Age",
            description: "Master electrical systems and complete automation",
            tasks: electric_tasks,
            unlocked_recipes: electric_unlocks,
        });
    }

    pub fn is_recipe_unlocked(&self, item: &CraftableItem) -> bool {
        for completed_tier in &self.completed_tiers {
            if let Some(tier_req) = self.tiers.get(completed_tier) {
                if tier_req.unlocked_recipes.contains(item) {
                    return true;
                }
            }
        }
        
        // Check current tier
        if let Some(current_tier_req) = self.tiers.get(&self.current_tier) {
            return current_tier_req.unlocked_recipes.contains(item);
        }
        
        false
    }

    pub fn update_task_progress(&mut self, task_type: &TaskType, amount: u32) {
        if let Some(current_tier_req) = self.tiers.get_mut(&self.current_tier) {
            for task in &mut current_tier_req.tasks {
                if std::mem::discriminant(&task.task_type) == std::mem::discriminant(task_type) {
                    match (&task.task_type, task_type) {
                        (TaskType::CollectResource(task_res), TaskType::CollectResource(update_res)) => {
                            if task_res == update_res {
                                task.update_progress(amount);
                            }
                        }
                        (TaskType::CraftItem(task_item), TaskType::CraftItem(update_item)) => {
                            if task_item == update_item {
                                task.update_progress(amount);
                            }
                        }
                        (TaskType::BuildStructure(task_building), TaskType::BuildStructure(update_building)) => {
                            if task_building == update_building {
                                task.update_progress(amount);
                            }
                        }
                        _ => {}
                    }
                }
            }
        }
        
        self.check_tier_completion();
    }

    fn check_tier_completion(&mut self) {
        if let Some(current_tier_req) = self.tiers.get(&self.current_tier) {
            let all_completed = current_tier_req.tasks.iter().all(|task| task.completed);
            
            if all_completed && !self.completed_tiers.contains(&self.current_tier) {
                self.completed_tiers.push(self.current_tier);
                
                if let Some(next_tier) = self.current_tier.next() {
                    self.current_tier = next_tier;
                }
            }
        }
    }

    pub fn get_current_tasks(&self) -> Vec<&Task> {
        if let Some(current_tier_req) = self.tiers.get(&self.current_tier) {
            current_tier_req.tasks.iter().collect()
        } else {
            Vec::new()
        }
    }

    pub fn get_incomplete_tasks(&self) -> Vec<&Task> {
        self.get_current_tasks()
            .into_iter()
            .filter(|task| !task.completed)
            .collect()
    }

    pub fn get_tier_progress(&self) -> (usize, usize) {
        if let Some(current_tier_req) = self.tiers.get(&self.current_tier) {
            let completed = current_tier_req.tasks.iter().filter(|task| task.completed).count();
            let total = current_tier_req.tasks.len();
            (completed, total)
        } else {
            (0, 0)
        }
    }
}