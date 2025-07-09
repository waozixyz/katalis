use raylib::prelude::*;
use crate::types::*;
use crate::world::World;
use crate::player::Player;
use crate::ui::{BuildingUI, InventoryUI};

pub fn handle_input(
    world: &mut World,
    camera: &Camera2D,
    rl: &RaylibHandle,
    player: &mut Player, // Changed to mutable
    building_ui: &mut BuildingUI,
    inventory_ui: &mut InventoryUI,
) -> bool { // Returns true if handled a building click
    // Check for building clicks first (on press, not hold)
    if rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT) {
        let mouse_screen_pos = rl.get_mouse_position();
        let mouse_world_pos = rl.get_screen_to_world2D(mouse_screen_pos, *camera);
        
        // Convert to tile coordinates
        let tile_x = (mouse_world_pos.x / TILE_SIZE as f32).floor() as usize;
        let tile_y = (mouse_world_pos.y / TILE_SIZE as f32).floor() as usize;
        
        // Check if there's a building at this position
        if let Some((origin_x, origin_y, building_type)) = world.get_building_origin_at(tile_x, tile_y) {
            // Open inventory UI in building mode
            inventory_ui.open_building((origin_x, origin_y), building_type);
            return true; // Handled the click
        }
        
        // Check for ground item collection (only if no building was clicked)
        if world.is_valid_position(tile_x, tile_y) {
            let distance = player.position.distance_to(mouse_world_pos);
            if distance <= MINING_RANGE {
                if let Some(ground_item) = world.collect_ground_item(tile_x, tile_y) {
                    player.inventory.add_resource(ground_item.resource_type, ground_item.amount);
                    return true; // Handled the click
                }
            }
        }
    }
    
    // Check for building demolition with right-click hold
    if rl.is_mouse_button_down(MouseButton::MOUSE_BUTTON_RIGHT) {
        let mouse_screen_pos = rl.get_mouse_position();
        let mouse_world_pos = rl.get_screen_to_world2D(mouse_screen_pos, *camera);
        
        // Convert to tile coordinates
        let tile_x = (mouse_world_pos.x / TILE_SIZE as f32).floor() as usize;
        let tile_y = (mouse_world_pos.y / TILE_SIZE as f32).floor() as usize;
        
        // Check if there's a building at this position
        if let Some((origin_x, origin_y, building_type)) = world.get_building_origin_at(tile_x, tile_y) {
            // Start demolition if not already demolishing
            if !player.is_demolishing() {
                player.start_demolition(building_type, (origin_x, origin_y));
            }
            return true; // Handled the click
        }
    }
    
    // Cancel demolition if right mouse button is released
    if rl.is_mouse_button_released(MouseButton::MOUSE_BUTTON_RIGHT) {
        player.cancel_demolition();
    }
    
    // Mining with left mouse button - support both click and hold
    if rl.is_mouse_button_down(MouseButton::MOUSE_BUTTON_LEFT) {
        let mouse_screen_pos = rl.get_mouse_position();
        let mouse_world_pos = rl.get_screen_to_world2D(mouse_screen_pos, *camera);
        
        // Check if target is within mining range
        let distance = player.position.distance_to(mouse_world_pos);
        if distance <= MINING_RANGE {
            // Try to start mining (this will only start if not already mining)
            world.try_start_mining(player, mouse_world_pos);
            
            // Store the target position for continuous mining
            player.set_mining_target(mouse_world_pos);
        }
    }
    
    // Stop continuous mining when mouse button is released
    if rl.is_mouse_button_released(MouseButton::MOUSE_BUTTON_LEFT) {
        player.clear_mining_target();
    }
    
    // Cancel mining immediately if right mouse button is pressed (but not if we're demolishing)
    if rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_RIGHT) && !player.is_demolishing() {
        player.current_mining = None;
        player.clear_mining_target();
    }
    
    false // No building was clicked
}