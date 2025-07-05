use raylib::prelude::*;
use super::{TileType, TreeType, VeinType};

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum ToolType {
    Pickaxe,
    Axe,
}

impl ToolType {
    pub fn get_name(&self) -> &'static str {
        match self {
            ToolType::Pickaxe => "Pickaxe",
            ToolType::Axe => "Axe",
        }
    }
    
    pub fn get_color(&self) -> Color {
        match self {
            ToolType::Pickaxe => Color::new(139, 69, 19, 255),
            ToolType::Axe => Color::new(160, 82, 45, 255),
        }
    }
    
    pub fn can_mine_vein(&self, vein_type: VeinType) -> bool {
        match self {
            ToolType::Pickaxe => true, // Pickaxe can mine all veins
            ToolType::Axe => false,    // Axes only cut trees
        }
    }
    
    pub fn can_mine_tree(&self) -> bool {
        matches!(self, ToolType::Axe)
    }
}

#[derive(Clone, Debug)]
pub struct MiningAction {
    pub tool_type: ToolType,
    pub target_position: Vector2,
    pub progress: f32,
    pub total_time: f32,
    pub target_type: MiningTarget,
}

#[derive(Clone, Debug, PartialEq)]
pub enum MiningTarget {
    ResourceVein(VeinType, usize, usize), // vein_type, tile_x, tile_y
    Tree(Vector2),
}

impl MiningAction {
    pub fn new(tool_type: ToolType, target_position: Vector2, target_type: MiningTarget) -> Self {
        Self {
            tool_type,
            target_position,
            progress: 0.0,
            total_time: super::MINING_TIME,
            target_type,
        }
    }
    
    pub fn update(&mut self, delta_time: f32) -> bool {
        self.progress += delta_time;
        let completed = self.progress >= self.total_time;
        if completed {
            println!("Mining action completed! Progress: {}/{}", self.progress, self.total_time);
        }
        completed
    }
    
    pub fn get_progress_percentage(&self) -> f32 {
        (self.progress / self.total_time).min(1.0)
    }
}