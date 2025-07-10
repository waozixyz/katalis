use raylib::prelude::*;
use crate::types::*;
use crate::animation::{SpriteAnimator, AnimationState, ActionType};
use crate::crafting::{CraftableItem, CraftingRecipe}; // ADD THIS LINE

pub struct Player {
    pub position: Vector2,
    pub size: f32,
    pub inventory: Inventory,
    pub facing_angle: f32,
    pub current_mining: Option<MiningAction>,
    pub mining_target: Option<Vector2>, // For continuous mining
    pub animator: SpriteAnimator,
    pub sprite_texture: Option<Texture2D>,
    pub last_movement: Vector2,
    pub scale: f32,
    pub facing_left: bool, 
    pub current_crafting: Option<CraftingProgress>,
    pub crafting_queue: Vec<QueuedCraft>,
    pub current_demolition: Option<DemolitionAction>,
    pub attack_timer: f32,
}

#[derive(Clone, Debug)]
pub struct CraftingProgress {
    pub item: CraftableItem,
    pub recipe: CraftingRecipe,
    pub progress: f32,
    pub total_time: f32,
}

#[derive(Clone, Debug)]
pub struct QueuedCraft {
    pub item: CraftableItem,
    pub quantity: u32,
}

#[derive(Clone, Debug)]
pub struct DemolitionAction {
    pub building_type: BuildingType,
    pub building_pos: (usize, usize),
    pub progress: f32,
    pub total_time: f32,
}


impl Player {
    pub fn new(x: f32, y: f32) -> Self {
        Self {
            position: Vector2::new(x, y),
            size: 20.0,
            inventory: Inventory::new(),
            facing_angle: 0.0,
            current_mining: None,
            mining_target: None,
            animator: SpriteAnimator::new(128, 192), // Assuming 32x32 sprite frames
            sprite_texture: None,
            last_movement: Vector2::zero(),
            scale: 0.3,
            facing_left: false,
            current_crafting: None,
            crafting_queue: Vec::new(),
            current_demolition: None,
            attack_timer: 0.0,
        }
    }

    pub fn start_crafting(&mut self, item: CraftableItem, recipe: CraftingRecipe) {
        if self.current_crafting.is_none() {
            println!("Started crafting: {}", item.get_name());
            let crafting_time = recipe.crafting_time;
            self.current_crafting = Some(CraftingProgress {
                item,
                recipe,
                progress: 0.0,
                total_time: crafting_time, 
            });
        }
    }
    
    pub fn add_to_crafting_queue(&mut self, item: CraftableItem, quantity: u32) {
        // Check if this item is already in queue and add to it
        for queued in &mut self.crafting_queue {
            if queued.item == item {
                queued.quantity += quantity;
                return;
            }
        }
        // If not found, add new entry
        self.crafting_queue.push(QueuedCraft { item, quantity });
    }
    
    pub fn is_crafting(&self) -> bool {
        self.current_crafting.is_some()
    }
    
    pub fn get_crafting_progress(&self) -> f32 {
        self.current_crafting.as_ref()
            .map(|c| c.progress / c.total_time)
            .unwrap_or(0.0)
    }
    
    pub fn get_current_crafting_item(&self) -> Option<&CraftableItem> {
        self.current_crafting.as_ref().map(|c| &c.item)
    }

    
    pub fn load_sprite(&mut self, texture: Texture2D) {
        self.sprite_texture = Some(texture);
    }

    pub fn update(&mut self, rl: &RaylibHandle, camera: &Camera2D) {
        let mut movement = Vector2::zero();
        
        // Player movement with WASD
        if rl.is_key_down(KeyboardKey::KEY_W) || rl.is_key_down(KeyboardKey::KEY_UP) {
            movement.y -= 1.0;
        }
        if rl.is_key_down(KeyboardKey::KEY_S) || rl.is_key_down(KeyboardKey::KEY_DOWN) {
            movement.y += 1.0;
        }
        if rl.is_key_down(KeyboardKey::KEY_A) || rl.is_key_down(KeyboardKey::KEY_LEFT) {
            movement.x -= 1.0;
        }
        if rl.is_key_down(KeyboardKey::KEY_D) || rl.is_key_down(KeyboardKey::KEY_RIGHT) {
            movement.x += 1.0;
        }
        
        // Store movement for animation
        self.last_movement = movement;
        
        // Update facing direction based on horizontal movement
        if movement.x < -0.1 {
            self.facing_left = true;
        } else if movement.x > 0.1 {
            self.facing_left = false;
        }
        // If moving only vertically, keep current facing direction
        
        // Cancel mining and demolition if player moves
        if movement.length() > 0.0 {
            if self.current_mining.is_some() {
                println!("Mining cancelled due to movement");
                self.current_mining = None;
            }
            if self.current_demolition.is_some() {
                println!("Demolition cancelled due to movement");
                self.current_demolition = None;
            }
        }
        
        // Normalize diagonal movement and apply directly (no rotation since camera rotation = 0)
        if movement.length() > 0.0 {
            movement = movement.normalized();
            
            // Since camera rotation is 0, no need to rotate movement
            self.position = self.position + movement * PLAYER_SPEED * rl.get_frame_time();
            
            // Update facing direction for animations
            self.animator.set_facing_direction(movement);
        }
        
        // Calculate facing angle based on mouse position (for tools/weapons)
        let mouse_screen_pos = rl.get_mouse_position();
        let mouse_world_pos = rl.get_screen_to_world2D(mouse_screen_pos, *camera);
        
        let direction = Vector2::new(
            mouse_world_pos.x - self.position.x,
            mouse_world_pos.y - self.position.y
        );
        
        self.facing_angle = direction.y.atan2(direction.x);
        
        // Update animation based on current state
        self.update_animation();
        
        // Update attack timer (decreases over time)
        if self.attack_timer > 0.0 {
            self.attack_timer -= rl.get_frame_time();
            if self.attack_timer < 0.0 {
                self.attack_timer = 0.0;
            }
        }
        
        // Update the animator
        self.animator.update(rl.get_frame_time());
    }
    
    fn update_animation(&mut self) {
        // Priority: Mining actions > Movement > Idle
        if let Some(mining) = &self.current_mining {
            // Determine action type based on mining target
            let action_type = match &mining.target_type {
                MiningTarget::Tree(_) => ActionType::Chopping,
                MiningTarget::ResourceVein(_, _, _) => ActionType::Digging,
            };
            
            let action_animation = self.animator.get_action_animation(action_type);
            self.animator.set_animation(action_animation);
        } else if self.last_movement.length() > 0.1 {
            // Player is moving
            let movement_animation = self.animator.get_movement_animation(self.last_movement);
            self.animator.set_animation(movement_animation);
        } else {
            // Player is idle
            self.animator.set_animation(AnimationState::Idle);
        }
    }
    
    pub fn start_mining(&mut self, mining_action: MiningAction) {
        println!("Started mining: {:?}", mining_action.target_type);
        self.current_mining = Some(mining_action);
    }
    
    pub fn is_mining(&self) -> bool {
        self.current_mining.is_some()
    }
    
    pub fn get_mining_progress(&self) -> f32 {
        self.current_mining.as_ref()
            .map(|m| m.get_progress_percentage())
            .unwrap_or(0.0)
    }
    
    pub fn set_mining_target(&mut self, target: Vector2) {
        self.mining_target = Some(target);
    }
    
    pub fn clear_mining_target(&mut self) {
        self.mining_target = None;
    }
    
    pub fn has_mining_target(&self) -> bool {
        self.mining_target.is_some()
    }
    
    pub fn get_mining_target(&self) -> Option<Vector2> {
        self.mining_target
    }
    
    pub fn start_demolition(&mut self, building_type: BuildingType, building_pos: (usize, usize)) {
        let demolition_time = 2.0; // 2 seconds to demolish
        println!("Started demolishing {:?} at ({}, {})", building_type, building_pos.0, building_pos.1);
        self.current_demolition = Some(DemolitionAction {
            building_type,
            building_pos,
            progress: 0.0,
            total_time: demolition_time,
        });
    }
    
    pub fn is_demolishing(&self) -> bool {
        self.current_demolition.is_some()
    }
    
    pub fn get_demolition_progress(&self) -> f32 {
        self.current_demolition.as_ref()
            .map(|d| d.progress / d.total_time)
            .unwrap_or(0.0)
    }
    
    pub fn update_demolition(&mut self, delta_time: f32) -> Option<DemolitionAction> {
        if let Some(demolition) = &mut self.current_demolition {
            demolition.progress += delta_time;
            if demolition.progress >= demolition.total_time {
                let completed = demolition.clone();
                self.current_demolition = None;
                return Some(completed);
            }
        }
        None
    }
    
    pub fn cancel_demolition(&mut self) {
        if self.current_demolition.is_some() {
            println!("Demolition cancelled");
            self.current_demolition = None;
        }
    }
    
    pub fn draw(&self, d: &mut RaylibMode2D<RaylibDrawHandle>) {
        if let Some(texture) = &self.sprite_texture {
            // Draw animated sprite with scaling and flipping
            let mut source_rect = self.animator.get_current_source_rect();
            
            // Flip the source rectangle horizontally if facing left
            if self.facing_left {
                source_rect.width = -source_rect.width; // Negative width flips horizontally
            }
            
            // Calculate scaled dimensions
            let sprite_width = self.animator.frame_width as f32 * self.scale;
            let sprite_height = self.animator.frame_height as f32 * self.scale;
            
            // Position the sprite so its center aligns with self.position
            let dest_rect = Rectangle::new(
                self.position.x - sprite_width * 0.5,  // Center horizontally
                self.position.y - sprite_height * 0.5, // Center vertically
                sprite_width,
                sprite_height,
            );
            
            d.draw_texture_pro(
                texture,
                source_rect,
                dest_rect,
                Vector2::zero(), // No additional origin offset needed since we already centered
                0.0, // No rotation
                Color::WHITE,
            );
        } else {
            // Fallback to old circle drawing (also scaled)
            let scaled_size = self.size * self.scale;
            d.draw_circle_v(self.position, scaled_size, Color::DARKBLUE);
            d.draw_circle_v(self.position, scaled_size - 4.0 * self.scale, Color::SKYBLUE);
            d.draw_circle_v(self.position, 3.0 * self.scale, Color::WHITE);
            
            // Draw direction indicator (also scaled)
            let direction_distance = scaled_size - 2.0 * self.scale;
            let front_pos = Vector2::new(
                self.position.x + direction_distance * self.facing_angle.cos(),
                self.position.y + direction_distance * self.facing_angle.sin()
            );
            d.draw_circle_v(front_pos, 2.0 * self.scale, Color::YELLOW);
            d.draw_line_v(self.position, front_pos, Color::new(255, 255, 0, 100));
        }
        
        // Draw progress bars (mining and crafting)
        let mut bar_y_offset = -30.0;
        
        // Draw mining progress bar
        if let Some(mining) = &self.current_mining {
            let progress = mining.get_progress_percentage();
            self.draw_progress_bar(d, progress, "Mining", Color::GREEN, bar_y_offset);
            bar_y_offset -= 15.0; // Stack bars vertically
        }
        
        // Draw crafting progress bar
        if let Some(crafting) = &self.current_crafting {
            let progress = crafting.progress / crafting.total_time;
            let label = format!("Crafting {}", crafting.item.get_name());
            self.draw_progress_bar(d, progress, &label, Color::BLUE, bar_y_offset);
            bar_y_offset -= 15.0; // Stack bars vertically
        }
        
        // Draw demolition progress bar
        if let Some(demolition) = &self.current_demolition {
            let progress = demolition.progress / demolition.total_time;
            let label = format!("Demolishing {}", demolition.building_type.get_name());
            self.draw_progress_bar(d, progress, &label, Color::RED, bar_y_offset);
        }
    }
    
    fn draw_progress_bar(&self, d: &mut RaylibMode2D<RaylibDrawHandle>, progress: f32, label: &str, color: Color, y_offset: f32) {
        let bar_width = 60.0;
        let bar_height = 6.0;
        let bar_x = self.position.x - bar_width * 0.5;
        let bar_y = self.position.y - (self.animator.frame_height as f32 * self.scale) * 0.5 + y_offset;
        
        // Background
        d.draw_rectangle(
            bar_x as i32,
            bar_y as i32,
            bar_width as i32,
            bar_height as i32,
            Color::new(100, 100, 100, 200)
        );
        
        // Progress
        d.draw_rectangle(
            bar_x as i32,
            bar_y as i32,
            (bar_width * progress) as i32,
            bar_height as i32,
            color
        );
        
        // Border
        d.draw_rectangle_lines(
            bar_x as i32,
            bar_y as i32,
            bar_width as i32,
            bar_height as i32,
            Color::WHITE
        );
        
        // Label above bar
        let text_width = 8 * label.len() as i32; // Approximate text width
        d.draw_text(
            label,
            (bar_x - text_width as f32 * 0.5 + bar_width * 0.5) as i32,
            (bar_y - 12.0) as i32,
            8,
            Color::WHITE
        );
    }
    
    pub fn set_scale(&mut self, new_scale: f32) {
        self.scale = new_scale;
    }
    
    pub fn get_scaled_size(&self) -> f32 {
        self.size * self.scale
    }
    
    pub fn is_attacking(&self) -> bool {
        self.attack_timer > 0.0
    }
    
    pub fn trigger_attack(&mut self) {
        self.attack_timer = 0.5; // Attack lasts for 0.5 seconds
    }
}