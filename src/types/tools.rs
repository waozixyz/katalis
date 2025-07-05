use raylib::prelude::*;
use super::{TileType, TreeType};
use crate::Player;

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
            ToolType::Pickaxe => Color::new(139, 69, 19, 255), // Brown handle
            ToolType::Axe => Color::new(160, 82, 45, 255), // Saddle brown
        }
    }
    
    pub fn can_mine_tile(&self, tile_type: TileType) -> bool {
        match self {
            ToolType::Pickaxe => matches!(tile_type, TileType::Stone | TileType::Iron | TileType::Coal),
            ToolType::Axe => false, // Axes don't mine tiles
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
    Tile(TileType),
    Tree(Vector2), // Changed: Use position instead of index
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
        self.progress >= self.total_time
    }
    
    pub fn get_progress_percentage(&self) -> f32 {
        (self.progress / self.total_time).min(1.0)
    }
}