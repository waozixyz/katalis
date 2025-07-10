use crate::types::*;
use crate::crafting::CraftableItem;
use raylib::prelude::*;
use std::collections::{HashMap, HashSet};

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Age {
    StoneAge = 1,
    EarlyCivilization = 2,
    IronAge = 3,
    IndustrialAge = 4,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum TechId {
    // Age 1: Stone Age
    BasicTools,
    FireControl,
    Hunting,
    FoodPreparation,
    StoneWorking,
    MaterialProcessing,
    
    // Age 2: Early Civilization  
    CopperWorking,
    BronzeMaking,
    BasicFarming,
    CropProcessing,
    BasicBuildings,
    SpecializedStructures,
    
    // Age 3: Iron Age
    IronSmelting,
    SteelMaking,
    GrainProcessing,
    BreadMaking,
    SimpleMachines,
    ComplexStructures,
    
    // Age 4: Industrial Age
    SteamBasics,
    SteamMachinery,
    BasicAutomation,
    ComplexAutomation,
    BasicElectronics,
    PowerSystems,
}

#[derive(Clone, Debug)]
pub struct TechObjective {
    pub description: String,
    pub objective_type: ObjectiveType,
    pub target: u32,
    pub current: u32,
    pub completed: bool,
}

#[derive(Clone, Debug)]
pub enum ObjectiveType {
    CollectResource(ResourceType),
    CraftItem(CraftableItem),
    BuildStructure(BuildingType),
    KillAnimals(u32), // Kill X animals
    ProcessItems(ResourceType, u32), // Process X items
    StoreItems(ResourceType, u32), // Have X items in storage
}

#[derive(Clone, Debug)]
pub struct TechBox {
    pub id: TechId,
    pub age: Age,
    pub name: &'static str,
    pub description: &'static str,
    pub icon: &'static str, // Icon filename
    pub objectives: Vec<TechObjective>,
    pub prerequisites: Vec<TechId>,
    pub unlocked_recipes: Vec<CraftableItem>,
    pub unlocked_buildings: Vec<BuildingType>,
    pub status: TechStatus,
    pub position: (f32, f32), // Position in tech tree UI
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum TechStatus {
    Locked,      // Prerequisites not met
    Available,   // Can be worked on
    InProgress,  // Some objectives completed
    Completed,   // All objectives done
}

pub struct TechTree {
    pub current_age: Age,
    pub techs: HashMap<TechId, TechBox>,
    pub completed_techs: HashSet<TechId>,
    pub is_open: bool, // Whether tech tree UI is open
}

impl Age {
    pub fn get_name(&self) -> &'static str {
        match self {
            Age::StoneAge => "Stone Age",
            Age::EarlyCivilization => "Early Civilization",
            Age::IronAge => "Iron Age", 
            Age::IndustrialAge => "Industrial Age",
        }
    }

    pub fn get_color(&self) -> Color {
        match self {
            Age::StoneAge => Color::new(139, 69, 19, 255), // Brown
            Age::EarlyCivilization => Color::new(205, 127, 50, 255), // Peru
            Age::IronAge => Color::LIGHTGRAY,
            Age::IndustrialAge => Color::new(70, 130, 180, 255), // Steel Blue
        }
    }

    pub fn next(&self) -> Option<Age> {
        match self {
            Age::StoneAge => Some(Age::EarlyCivilization),
            Age::EarlyCivilization => Some(Age::IronAge),
            Age::IronAge => Some(Age::IndustrialAge),
            Age::IndustrialAge => None,
        }
    }

    pub fn get_required_techs(&self) -> Vec<TechId> {
        match self {
            Age::StoneAge => vec![], // Starting age
            Age::EarlyCivilization => vec![
                TechId::BasicTools,
                TechId::FireControl,
                TechId::Hunting,
                TechId::FoodPreparation,
                TechId::StoneWorking,
                TechId::MaterialProcessing,
            ],
            Age::IronAge => vec![
                TechId::CopperWorking,
                TechId::BronzeMaking,
                TechId::BasicFarming,
                TechId::CropProcessing,
                TechId::BasicBuildings,
                TechId::SpecializedStructures,
            ],
            Age::IndustrialAge => vec![
                TechId::BreadMaking, // Key requirement!
                TechId::IronSmelting,
                TechId::SteelMaking,
                TechId::SimpleMachines,
            ],
        }
    }
}

impl TechObjective {
    pub fn new(description: &str, objective_type: ObjectiveType, target: u32) -> Self {
        Self {
            description: description.to_string(),
            objective_type,
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

impl TechBox {
    pub fn get_completion_ratio(&self) -> f32 {
        if self.objectives.is_empty() {
            return 1.0;
        }
        let completed = self.objectives.iter().filter(|obj| obj.completed).count() as f32;
        completed / self.objectives.len() as f32
    }

    pub fn is_complete(&self) -> bool {
        self.objectives.iter().all(|obj| obj.completed)
    }

    pub fn get_status_color(&self) -> Color {
        match self.status {
            TechStatus::Locked => Color::new(100, 100, 100, 255), // Gray
            TechStatus::Available => Color::new(100, 150, 255, 255), // Blue
            TechStatus::InProgress => Color::new(255, 200, 100, 255), // Yellow
            TechStatus::Completed => Color::new(100, 255, 100, 255), // Green
        }
    }
}

impl TechTree {
    pub fn new() -> Self {
        let mut tree = Self {
            current_age: Age::StoneAge,
            techs: HashMap::new(),
            completed_techs: HashSet::new(),
            is_open: false,
        };
        
        tree.initialize_tech_tree();
        tree.update_tech_status();
        tree
    }

    fn initialize_tech_tree(&mut self) {
        // Age 1: Stone Age Techs
        
        // Basic Tools - Starting tech
        let basic_tools = TechBox {
            id: TechId::BasicTools,
            age: Age::StoneAge,
            name: "Basic Tools",
            description: "Learn to craft basic stone tools for survival",
            icon: "stone_pickaxe.png",
            objectives: vec![
                TechObjective::new("Collect Rocks", ObjectiveType::CollectResource(ResourceType::Rocks), 20),
                TechObjective::new("Collect Sticks", ObjectiveType::CollectResource(ResourceType::Sticks), 15),
                TechObjective::new("Craft Stone Pickaxe", ObjectiveType::CraftItem(CraftableItem::StonePickaxe), 1),
                TechObjective::new("Craft Stone Axe", ObjectiveType::CraftItem(CraftableItem::StoneAxe), 1),
            ],
            prerequisites: vec![],
            unlocked_recipes: vec![
                CraftableItem::StonePickaxe,
                CraftableItem::StoneAxe,
                CraftableItem::StoneShovel,
                CraftableItem::StoneSword,
            ],
            unlocked_buildings: vec![],
            status: TechStatus::Available,
            position: (100.0, 100.0),
        };
        
        // Fire Control
        let fire_control = TechBox {
            id: TechId::FireControl,
            age: Age::StoneAge,
            name: "Fire Control", 
            description: "Master fire for cooking and material processing",
            icon: "campfire.png",
            objectives: vec![
                TechObjective::new("Build Campfire", ObjectiveType::BuildStructure(BuildingType::Campfire), 1),
                TechObjective::new("Build Charcoal Pit", ObjectiveType::BuildStructure(BuildingType::CharcoalPit), 1),
                TechObjective::new("Make Charcoal", ObjectiveType::CraftItem(CraftableItem::Charcoal), 5),
            ],
            prerequisites: vec![TechId::BasicTools],
            unlocked_recipes: vec![
                CraftableItem::Campfire,
                CraftableItem::CharcoalPit,
                CraftableItem::Charcoal,
            ],
            unlocked_buildings: vec![BuildingType::Campfire, BuildingType::CharcoalPit],
            status: TechStatus::Locked,
            position: (300.0, 100.0),
        };
        
        // Hunting
        let hunting = TechBox {
            id: TechId::Hunting,
            age: Age::StoneAge,
            name: "Hunting",
            description: "Learn to hunt animals for food",
            icon: "stone_sword.png",
            objectives: vec![
                TechObjective::new("Craft Stone Sword", ObjectiveType::CraftItem(CraftableItem::StoneSword), 1),
                TechObjective::new("Kill Chickens", ObjectiveType::KillAnimals(3), 3),
                TechObjective::new("Collect Raw Chicken", ObjectiveType::CollectResource(ResourceType::RawChicken), 5),
            ],
            prerequisites: vec![TechId::BasicTools],
            unlocked_recipes: vec![],
            unlocked_buildings: vec![],
            status: TechStatus::Locked,
            position: (100.0, 250.0),
        };
        
        // Food Preparation
        let food_preparation = TechBox {
            id: TechId::FoodPreparation,
            age: Age::StoneAge,
            name: "Food Preparation",
            description: "Cook and store food for survival",
            icon: "cooked_chicken.png",
            objectives: vec![
                TechObjective::new("Cook Chicken", ObjectiveType::CraftItem(CraftableItem::CookedChicken), 3),
                TechObjective::new("Collect Plant Fiber", ObjectiveType::CollectResource(ResourceType::PlantFiber), 10),
                TechObjective::new("Store Cooked Food", ObjectiveType::StoreItems(ResourceType::CookedChicken, 10), 10),
            ],
            prerequisites: vec![TechId::FireControl, TechId::Hunting],
            unlocked_recipes: vec![CraftableItem::CookedChicken],
            unlocked_buildings: vec![],
            status: TechStatus::Locked,
            position: (500.0, 175.0),
        };
        
        // Stone Working
        let stone_working = TechBox {
            id: TechId::StoneWorking,
            age: Age::StoneAge,
            name: "Stone Working",
            description: "Advanced stone crafting and tool making",
            icon: "stone_anvil.png",
            objectives: vec![
                TechObjective::new("Collect Stone", ObjectiveType::CollectResource(ResourceType::Stone), 50),
                TechObjective::new("Build Stone Anvil", ObjectiveType::BuildStructure(BuildingType::StoneAnvil), 1),
                TechObjective::new("Craft Stone Tools", ObjectiveType::CraftItem(CraftableItem::StonePickaxe), 10), // Any 10 stone tools
            ],
            prerequisites: vec![TechId::BasicTools],
            unlocked_recipes: vec![CraftableItem::StoneAnvil],
            unlocked_buildings: vec![BuildingType::StoneAnvil],
            status: TechStatus::Locked,
            position: (100.0, 400.0),
        };
        
        // Material Processing
        let material_processing = TechBox {
            id: TechId::MaterialProcessing,
            age: Age::StoneAge,
            name: "Material Processing",
            description: "Process raw materials into useful components",
            icon: "wooden_planks.png",
            objectives: vec![
                TechObjective::new("Process Wood to Planks", ObjectiveType::CraftItem(CraftableItem::WoodenPlanks), 5),
                TechObjective::new("Create Wooden Beams", ObjectiveType::CraftItem(CraftableItem::WoodenBeams), 3),
                TechObjective::new("Make Wooden Gears", ObjectiveType::CraftItem(CraftableItem::WoodenGears), 2),
                TechObjective::new("Build Storage", ObjectiveType::StoreItems(ResourceType::Wood, 50), 50),
            ],
            prerequisites: vec![TechId::StoneWorking, TechId::FireControl],
            unlocked_recipes: vec![
                CraftableItem::WoodenPlanks,
                CraftableItem::WoodenBeams,
                CraftableItem::WoodenGears,
                CraftableItem::WoodenFrames,
            ],
            unlocked_buildings: vec![],
            status: TechStatus::Locked,
            position: (300.0, 350.0),
        };

        // Insert all Age 1 techs
        self.techs.insert(TechId::BasicTools, basic_tools);
        self.techs.insert(TechId::FireControl, fire_control);
        self.techs.insert(TechId::Hunting, hunting);
        self.techs.insert(TechId::FoodPreparation, food_preparation);
        self.techs.insert(TechId::StoneWorking, stone_working);
        self.techs.insert(TechId::MaterialProcessing, material_processing);

        // Age 2: Early Civilization (placeholder for now)
        let copper_working = TechBox {
            id: TechId::CopperWorking,
            age: Age::EarlyCivilization,
            name: "Copper Working",
            description: "Learn to smelt and work with copper",
            icon: "copper_ingots.png",
            objectives: vec![
                TechObjective::new("Build Crude Furnace", ObjectiveType::BuildStructure(BuildingType::CrudeFurnace), 1),
                TechObjective::new("Smelt Copper Ingots", ObjectiveType::CraftItem(CraftableItem::CopperIngots), 5),
                TechObjective::new("Make Copper Wire", ObjectiveType::CraftItem(CraftableItem::CopperWire), 10),
            ],
            prerequisites: vec![], // Will be updated when age advances
            unlocked_recipes: vec![
                CraftableItem::CrudeFurnace,
                CraftableItem::CopperIngots,
                CraftableItem::CopperPlates,
                CraftableItem::CopperWire,
            ],
            unlocked_buildings: vec![BuildingType::CrudeFurnace],
            status: TechStatus::Locked,
            position: (100.0, 600.0),
        };
        
        self.techs.insert(TechId::CopperWorking, copper_working);
        
        // Add more Age 2+ techs as needed...
    }

    pub fn toggle_ui(&mut self) {
        self.is_open = !self.is_open;
    }

    pub fn update_tech_status(&mut self) {
        for tech in self.techs.values_mut() {
            // Check if prerequisites are met
            let prereqs_met = tech.prerequisites.iter()
                .all(|prereq_id| self.completed_techs.contains(prereq_id));
            
            // Check if tech is from current age or earlier
            let age_available = tech.age as u32 <= self.current_age as u32;
            
            if !prereqs_met || !age_available {
                tech.status = TechStatus::Locked;
            } else if tech.is_complete() {
                tech.status = TechStatus::Completed;
                self.completed_techs.insert(tech.id);
            } else if tech.objectives.iter().any(|obj| obj.current > 0) {
                tech.status = TechStatus::InProgress;
            } else {
                tech.status = TechStatus::Available;
            }
        }
        
        // Check if we can advance to next age
        if let Some(next_age) = self.current_age.next() {
            let required_techs = self.current_age.get_required_techs();
            let all_required_complete = required_techs.iter()
                .all(|tech_id| self.completed_techs.contains(tech_id));
            
            if all_required_complete {
                self.current_age = next_age;
                // Re-run status update for new age
                self.update_tech_status();
            }
        }
    }

    pub fn is_recipe_unlocked(&self, recipe: &CraftableItem) -> bool {
        // Check if any completed tech unlocks this recipe
        for tech_id in &self.completed_techs {
            if let Some(tech) = self.techs.get(tech_id) {
                if tech.status == TechStatus::Completed && tech.unlocked_recipes.contains(recipe) {
                    return true;
                }
            }
        }
        false
    }

    pub fn is_building_unlocked(&self, building: &BuildingType) -> bool {
        // Check if any completed tech unlocks this building
        for tech_id in &self.completed_techs {
            if let Some(tech) = self.techs.get(tech_id) {
                if tech.status == TechStatus::Completed && tech.unlocked_buildings.contains(building) {
                    return true;
                }
            }
        }
        false
    }

    pub fn update_objective_progress(&mut self, objective_type: &ObjectiveType, amount: u32) {
        for tech in self.techs.values_mut() {
            if tech.status == TechStatus::Locked || tech.status == TechStatus::Completed {
                continue;
            }
            
            for objective in &mut tech.objectives {
                if std::mem::discriminant(&objective.objective_type) == std::mem::discriminant(objective_type) {
                    match (&objective.objective_type, objective_type) {
                        (ObjectiveType::CollectResource(obj_res), ObjectiveType::CollectResource(update_res)) => {
                            if obj_res == update_res {
                                objective.update_progress(amount);
                            }
                        }
                        (ObjectiveType::CraftItem(obj_item), ObjectiveType::CraftItem(update_item)) => {
                            if obj_item == update_item {
                                objective.update_progress(amount);
                            }
                        }
                        (ObjectiveType::BuildStructure(obj_building), ObjectiveType::BuildStructure(update_building)) => {
                            if obj_building == update_building {
                                objective.update_progress(amount);
                            }
                        }
                        (ObjectiveType::KillAnimals(_), ObjectiveType::KillAnimals(_)) => {
                            objective.update_progress(amount);
                        }
                        _ => {}
                    }
                }
            }
        }
        
        self.update_tech_status();
    }

    pub fn get_current_objectives(&self) -> Vec<&TechBox> {
        let mut current_techs: Vec<&TechBox> = self.techs.values()
            .filter(|tech| tech.status == TechStatus::Available || tech.status == TechStatus::InProgress)
            .collect();
        
        // Sort by progress (in-progress first, then available)
        current_techs.sort_by(|a, b| {
            match (a.status, b.status) {
                (TechStatus::InProgress, TechStatus::Available) => std::cmp::Ordering::Less,
                (TechStatus::Available, TechStatus::InProgress) => std::cmp::Ordering::Greater,
                _ => std::cmp::Ordering::Equal,
            }
        });
        
        current_techs
    }

    pub fn get_techs_for_age(&self, age: Age) -> Vec<&TechBox> {
        self.techs.values()
            .filter(|tech| tech.age == age)
            .collect()
    }
}