use raylib::prelude::*;
use crate::types::*;
use crate::world::World;
use crate::player::Player;
use crate::assets::AssetManager;

// FIXED: Function to calculate visible world bounds
fn get_visible_bounds(camera: &Camera2D, screen_width: i32, screen_height: i32) -> (Vector2, Vector2) {
    // Calculate the screen corners in world space
    let screen_corners = [
        Vector2::new(0.0, 0.0),
        Vector2::new(screen_width as f32, 0.0),
        Vector2::new(0.0, screen_height as f32),
        Vector2::new(screen_width as f32, screen_height as f32),
    ];
    
    // Convert screen corners to world space using a simpler approach
    let mut world_corners = Vec::new();
    for corner in screen_corners.iter() {
        // Translate relative to camera offset
        let relative_pos = Vector2::new(
            corner.x - camera.offset.x,
            corner.y - camera.offset.y,
        );
        
        // Apply inverse zoom
        let scaled_pos = Vector2::new(
            relative_pos.x / camera.zoom,
            relative_pos.y / camera.zoom,
        );
        
        // Apply inverse rotation
        let rotation_rad = camera.rotation * std::f32::consts::PI / 180.0;
        let rotated_pos = Vector2::new(
            scaled_pos.x * rotation_rad.cos() + scaled_pos.y * rotation_rad.sin(),
            -scaled_pos.x * rotation_rad.sin() + scaled_pos.y * rotation_rad.cos(),
        );
        
        // Translate to world space
        let world_pos = Vector2::new(
            rotated_pos.x + camera.target.x,
            rotated_pos.y + camera.target.y,
        );
        
        world_corners.push(world_pos);
    }
    
    // Find the bounding box of all world corners
    let min_x = world_corners.iter().map(|v| v.x).fold(f32::INFINITY, f32::min);
    let max_x = world_corners.iter().map(|v| v.x).fold(f32::NEG_INFINITY, f32::max);
    let min_y = world_corners.iter().map(|v| v.y).fold(f32::INFINITY, f32::min);
    let max_y = world_corners.iter().map(|v| v.y).fold(f32::NEG_INFINITY, f32::max);
    
    // Add extra padding to ensure we don't miss any tiles at the edges
    let padding = TILE_SIZE as f32 * 3.0;  // INCREASED: More padding for safety
    
    (
        Vector2::new(min_x - padding, min_y - padding),
        Vector2::new(max_x + padding, max_y + padding)
    )
}

// NEW: Helper function to convert screen coordinates to world coordinates
// (Since we can't access rl.get_screen_to_world2D here)
fn screen_to_world_2d(position: Vector2, camera: Camera2D) -> Vector2 {
    // This is a simplified version of raylib's screen_to_world_2d
    let matrix = get_camera_matrix_2d(camera);
    let inverted_matrix = matrix_invert(matrix);
    
    // Transform the position
    let world_pos = vector2_transform(position, inverted_matrix);
    world_pos
}

// NEW: Helper functions for matrix math (simplified versions)
fn get_camera_matrix_2d(camera: Camera2D) -> Matrix {
    let matOrigin = matrix_translate(-camera.target.x, -camera.target.y, 0.0);
    let matRotation = matrix_rotate_z(camera.rotation * std::f32::consts::PI / 180.0);
    let matScale = matrix_scale(camera.zoom, camera.zoom, 1.0);
    let matTranslation = matrix_translate(camera.offset.x, camera.offset.y, 0.0);
    
    let result = matrix_multiply(matrix_multiply(matrix_multiply(matOrigin, matScale), matRotation), matTranslation);
    result
}

fn matrix_invert(mat: Matrix) -> Matrix {
    // Simplified matrix inversion for 2D case
    // For a proper implementation, you'd use the full 4x4 matrix inversion
    // This is a basic approximation
    Matrix {
        m0: mat.m0, m4: mat.m4, m8: mat.m8, m12: -mat.m12,
        m1: mat.m1, m5: mat.m5, m9: mat.m9, m13: -mat.m13,
        m2: mat.m2, m6: mat.m6, m10: mat.m10, m14: mat.m14,
        m3: mat.m3, m7: mat.m7, m11: mat.m11, m15: mat.m15,
    }
}

fn matrix_translate(x: f32, y: f32, z: f32) -> Matrix {
    Matrix {
        m0: 1.0, m4: 0.0, m8: 0.0, m12: x,
        m1: 0.0, m5: 1.0, m9: 0.0, m13: y,
        m2: 0.0, m6: 0.0, m10: 1.0, m14: z,
        m3: 0.0, m7: 0.0, m11: 0.0, m15: 1.0,
    }
}

fn matrix_rotate_z(angle: f32) -> Matrix {
    let cos_angle = angle.cos();
    let sin_angle = angle.sin();
    
    Matrix {
        m0: cos_angle, m4: -sin_angle, m8: 0.0, m12: 0.0,
        m1: sin_angle, m5: cos_angle, m9: 0.0, m13: 0.0,
        m2: 0.0, m6: 0.0, m10: 1.0, m14: 0.0,
        m3: 0.0, m7: 0.0, m11: 0.0, m15: 1.0,
    }
}

fn matrix_scale(x: f32, y: f32, z: f32) -> Matrix {
    Matrix {
        m0: x, m4: 0.0, m8: 0.0, m12: 0.0,
        m1: 0.0, m5: y, m9: 0.0, m13: 0.0,
        m2: 0.0, m6: 0.0, m10: z, m14: 0.0,
        m3: 0.0, m7: 0.0, m11: 0.0, m15: 1.0,
    }
}

fn matrix_multiply(left: Matrix, right: Matrix) -> Matrix {
    Matrix {
        m0: left.m0*right.m0 + left.m1*right.m4 + left.m2*right.m8 + left.m3*right.m12,
        m1: left.m0*right.m1 + left.m1*right.m5 + left.m2*right.m9 + left.m3*right.m13,
        m2: left.m0*right.m2 + left.m1*right.m6 + left.m2*right.m10 + left.m3*right.m14,
        m3: left.m0*right.m3 + left.m1*right.m7 + left.m2*right.m11 + left.m3*right.m15,
        m4: left.m4*right.m0 + left.m5*right.m4 + left.m6*right.m8 + left.m7*right.m12,
        m5: left.m4*right.m1 + left.m5*right.m5 + left.m6*right.m9 + left.m7*right.m13,
        m6: left.m4*right.m2 + left.m5*right.m6 + left.m6*right.m10 + left.m7*right.m14,
        m7: left.m4*right.m3 + left.m5*right.m7 + left.m6*right.m11 + left.m7*right.m15,
        m8: left.m8*right.m0 + left.m9*right.m4 + left.m10*right.m8 + left.m11*right.m12,
        m9: left.m8*right.m1 + left.m9*right.m5 + left.m10*right.m9 + left.m11*right.m13,
        m10: left.m8*right.m2 + left.m9*right.m6 + left.m10*right.m10 + left.m11*right.m14,
        m11: left.m8*right.m3 + left.m9*right.m7 + left.m10*right.m11 + left.m11*right.m15,
        m12: left.m12*right.m0 + left.m13*right.m4 + left.m14*right.m8 + left.m15*right.m12,
        m13: left.m12*right.m1 + left.m13*right.m5 + left.m14*right.m9 + left.m15*right.m13,
        m14: left.m12*right.m2 + left.m13*right.m6 + left.m14*right.m10 + left.m15*right.m14,
        m15: left.m12*right.m3 + left.m13*right.m7 + left.m14*right.m11 + left.m15*right.m15,
    }
}

fn vector2_transform(v: Vector2, mat: Matrix) -> Vector2 {
    Vector2::new(
        mat.m0*v.x + mat.m4*v.y + mat.m12,
        mat.m1*v.x + mat.m5*v.y + mat.m13
    )
}

// NEW: Check if a tile is visible
fn is_tile_visible(tile_x: usize, tile_y: usize, min_bound: Vector2, max_bound: Vector2) -> bool {
    let tile_world_x = tile_x as f32 * TILE_SIZE as f32;
    let tile_world_y = tile_y as f32 * TILE_SIZE as f32;
    let tile_world_x_end = tile_world_x + TILE_SIZE as f32;
    let tile_world_y_end = tile_world_y + TILE_SIZE as f32;
    
    // Check if tile bounds overlap with visible bounds
    !(tile_world_x_end < min_bound.x || 
      tile_world_x > max_bound.x || 
      tile_world_y_end < min_bound.y || 
      tile_world_y > max_bound.y)
}
pub fn draw_world(d: &mut RaylibMode2D<RaylibDrawHandle>, world: &World, camera: &Camera2D, assets: &AssetManager) {
    // Calculate visible bounds
    let (min_bound, max_bound) = get_visible_bounds(camera, 1200, 800);
    
    // Calculate tile range to check (with some padding)
    let start_x = ((min_bound.x / TILE_SIZE as f32).floor() as i32).max(0) as usize;
    let end_x = ((max_bound.x / TILE_SIZE as f32).ceil() as i32).min(world.width as i32) as usize;
    let start_y = ((min_bound.y / TILE_SIZE as f32).floor() as i32).max(0) as usize;
    let end_y = ((max_bound.y / TILE_SIZE as f32).ceil() as i32).min(world.height as i32) as usize;
    
    // Draw terrain tiles
    for x in start_x..end_x {
        for y in start_y..end_y {
            if !is_tile_visible(x, y, min_bound, max_bound) {
                continue;
            }
            
            let tile = &world.tiles[x][y];
            let pos_x = x as i32 * TILE_SIZE;
            let pos_y = y as i32 * TILE_SIZE;
            
            // Draw base terrain - with texture if available, otherwise solid color
            if let Some(terrain_texture) = assets.get_terrain_texture(tile.tile_type) {
                // Draw terrain texture
                d.draw_texture_pro(
                    terrain_texture,
                    Rectangle::new(0.0, 0.0, terrain_texture.width as f32, terrain_texture.height as f32),
                    Rectangle::new(pos_x as f32, pos_y as f32, TILE_SIZE as f32, TILE_SIZE as f32),
                    Vector2::zero(),
                    0.0,
                    Color::WHITE
                );
            } else {
                // Fallback to solid color
                let color = tile.tile_type.get_color();
                d.draw_rectangle(pos_x, pos_y, TILE_SIZE, TILE_SIZE, color);
            }
            
            // Draw resource veins with PNG assets or fallback to colored overlays
            if let Some(vein) = &tile.resource_vein {
                if let Some(resource_texture) = assets.get_resource_texture(vein.vein_type) {
                    // Draw PNG texture centered and scaled
                    let richness_percentage = vein.get_richness_percentage();
                    let alpha = (255.0 * (0.5 + richness_percentage * 0.5)) as u8;
                    let tint = Color::new(255, 255, 255, alpha);
                    
                    // Center the resource texture at 24x24 pixels
                    let texture_size = 24.0;
                    let offset = (TILE_SIZE as f32 - texture_size) * 0.5;
                    
                    d.draw_texture_pro(
                        resource_texture,
                        Rectangle::new(0.0, 0.0, resource_texture.width as f32, resource_texture.height as f32),
                        Rectangle::new(
                            pos_x as f32 + offset,
                            pos_y as f32 + offset,
                            texture_size,
                            texture_size
                        ),
                        Vector2::zero(),
                        0.0,
                        tint
                    );
                } else {
                    // Fallback to colored overlay
                    let richness_percentage = vein.get_richness_percentage();
                    let mut overlay_color = vein.vein_type.get_overlay_color();
                    overlay_color.a = (overlay_color.a as f32 * (0.3 + richness_percentage * 0.7)) as u8;
                    
                    d.draw_rectangle(pos_x + 2, pos_y + 2, TILE_SIZE - 4, TILE_SIZE - 4, overlay_color);
                    
                    // Draw richness indicator dots
                    let dots = (richness_percentage * 4.0) as i32;
                    for i in 0..dots {
                        let dot_x = pos_x + 4 + (i % 2) * 12;
                        let dot_y = pos_y + 4 + (i / 2) * 12;
                        d.draw_circle(dot_x, dot_y, 2.0, Color::WHITE);
                    }
                }
            }
            
            // Grid lines (subtle) - only if no terrain texture
            if !assets.has_terrain_texture(tile.tile_type) {
                d.draw_rectangle_lines(pos_x, pos_y, TILE_SIZE, TILE_SIZE, Color::new(255, 255, 255, 20));
            }
        }
    }
    
    // Rest of the function remains the same...
    // Draw trees, day/night overlay, etc.
    let visible_trees = world.get_trees_in_bounds(min_bound, max_bound);
    for tree in visible_trees {
        tree.draw(d);
    }
    
    // Apply day/night overlay
    let light_level = world.game_time.get_light_level();
    if light_level < 1.0 {
        let darkness = (255.0 * (1.0 - light_level)) as u8;
        let night_color = Color::new(0, 0, 50, darkness);
        
        for x in start_x..end_x {
            for y in start_y..end_y {
                if !is_tile_visible(x, y, min_bound, max_bound) {
                    continue;
                }
                
                let pos_x = x as i32 * TILE_SIZE;
                let pos_y = y as i32 * TILE_SIZE;
                d.draw_rectangle(pos_x, pos_y, TILE_SIZE, TILE_SIZE, night_color);
            }
        }
    }
}

pub fn draw_ui(d: &mut RaylibDrawHandle, player: &Player) {
    // Background panel with Factorio-style colors
    d.draw_rectangle(10, 10, 320, 120, Color::new(47, 47, 47, 220));
    d.draw_rectangle_lines(10, 10, 320, 120, Color::new(150, 150, 150, 255));
    
    // Title
    d.draw_text("Katalis - Factory Builder", 20, 20, 20, Color::new(255, 255, 255, 255));
    
    // Controls
    d.draw_text("WASD: Move Player", 20, 45, 16, Color::new(200, 200, 200, 255));
    d.draw_text("LMB: Auto-mine (Pickaxe/Axe/Shovel)", 20, 65, 16, Color::new(200, 200, 200, 255));
    d.draw_text("RMB: Cancel mining", 20, 85, 16, Color::new(200, 200, 200, 255));
    d.draw_text("Shift+IJKL: Free Camera", 20, 105, 16, Color::new(200, 200, 200, 255));
    
    // Player position
    d.draw_text(
        &format!("Position: ({:.0}, {:.0})", player.position.x, player.position.y),
        20,
        135,
        14,
        Color::LIGHTGRAY
    );
    
    // Mining status
    if player.is_mining() {
        let progress = (player.get_mining_progress() * 100.0) as i32;
        d.draw_text(
            &format!("Mining: {}%", progress),
            20,
            155,
            14,
            Color::GREEN
        );
    }
    
    // Draw inventory at top right
    draw_inventory(d, &player.inventory);
}

// Update the inventory drawing function
fn draw_inventory(d: &mut RaylibDrawHandle, inventory: &Inventory) {
    let inv_x = 1200 - 200;
    let inv_y = 10;
    let inv_width = 180;
    let inv_height = 180; // Increased height for clay
    
    // Inventory background
    d.draw_rectangle(inv_x, inv_y, inv_width, inv_height, Color::new(47, 47, 47, 220));
    d.draw_rectangle_lines(inv_x, inv_y, inv_width, inv_height, Color::new(150, 150, 150, 255));
    
    // Inventory title
    d.draw_text("Inventory", inv_x + 10, inv_y + 10, 18, Color::WHITE);
    
    // Draw resources
    let resources = [ResourceType::Wood, ResourceType::Stone, ResourceType::IronOre, ResourceType::Coal, ResourceType::Clay, ResourceType::CopperOre];
    for (i, resource) in resources.iter().enumerate() {
        let y_offset = inv_y + 35 + (i as i32 * 20); // Reduced spacing to fit more resources
        let amount = inventory.get_amount(resource);
        let color = resource.get_color();
        
        // Resource icon (simple colored rectangle)
        d.draw_rectangle(inv_x + 10, y_offset, 15, 15, color);
        d.draw_rectangle_lines(inv_x + 10, y_offset, 15, 15, Color::WHITE);
        
        // Resource name and amount
        d.draw_text(
            &format!("{}: {}", resource.get_name(), amount),
            inv_x + 30,
            y_offset,
            13, // Smaller font to fit more resources
            Color::WHITE
        );
    }
}


pub fn draw_time_display(d: &mut RaylibDrawHandle, game_time: &GameTime) {
    let time_str = game_time.get_time_string();
    let is_day = game_time.is_day();
    
    // Center of screen
    let x = 1200 / 2 - 40;
    let y = 30;
    
    // Background
    d.draw_rectangle(x - 10, y - 5, 80, 30, Color::new(47, 47, 47, 220));
    d.draw_rectangle_lines(x - 10, y - 5, 80, 30, Color::new(150, 150, 150, 255));
    
    // Time text
    let color = if is_day { Color::YELLOW } else { Color::new(100, 100, 255, 255) };
    d.draw_text(&time_str, x, y, 20, color);
    
    // Day/Night indicator
    let indicator = if is_day { "☀" } else { "🌙" };
    d.draw_text(indicator, x + 65, y, 16, color);
}
