// UI module organization
pub mod common;
pub mod inventory_layout;
pub mod crafting_queue;
pub mod inventory;
pub mod building;
pub mod pause_menu;

// Re-export all UI components for easy access
pub use crafting_queue::CraftingQueueUI;
pub use inventory::InventoryUI;
pub use building::BuildingUI;
pub use pause_menu::PauseMenu;