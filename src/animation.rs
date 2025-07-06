use raylib::prelude::*;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum AnimationState {
    Idle,
    WalkingUp,
    WalkingDown,
    WalkingLeft,
    WalkingRight,
    WalkingUpLeft,
    WalkingUpRight,
    WalkingDownLeft,
    WalkingDownRight,
    ChoppingUp,
    ChoppingDown,
    ChoppingLeft,
    ChoppingRight,
    DiggingUp,
    DiggingDown,
    DiggingLeft,
    DiggingRight,
    SwingingSwordUp,
    SwingingSwordDown,
    SwingingSwordLeft,
    SwingingSwordRight,
}

#[derive(Clone, Debug)]
pub struct AnimationFrame {
    pub row: i32,           // Which row in the spritesheet
    pub start_frame: i32,   // Starting frame in that row
    pub frame_count: i32,   // How many frames in this animation
    pub frame_duration: f32, // How long each frame lasts (in seconds)
    pub loop_animation: bool, // Should this animation loop?
}

#[derive(Clone, Debug)]
pub struct SpriteAnimator {
    pub current_state: AnimationState,
    pub current_frame: i32,
    pub frame_timer: f32,
    pub animations: std::collections::HashMap<AnimationState, AnimationFrame>,
    pub frame_width: i32,
    pub frame_height: i32,
    pub facing_direction: Vector2, // For determining which direction to face
}

impl AnimationFrame {
    pub fn new(row: i32, start_frame: i32, frame_count: i32, frame_duration: f32, loop_animation: bool) -> Self {
        Self {
            row,
            start_frame,
            frame_count,
            frame_duration,
            loop_animation,
        }
    }
}

impl SpriteAnimator {
    pub fn new(frame_width: i32, frame_height: i32) -> Self {
        let mut animations = std::collections::HashMap::new();
        
        // Define all animations
        // Row 0: 3 idle frames
        // Row 1: 2 walking frames
        
        // Idle (row 0, 3 frames)
        animations.insert(AnimationState::Idle, 
            AnimationFrame::new(0, 0, 3, 0.5, true));
        
        // Walking animations - using row 1 with 2 frames
        animations.insert(AnimationState::WalkingUp, 
            AnimationFrame::new(1, 0, 2, 0.3, true)); // Row 1, 2 frames, slower pace
        animations.insert(AnimationState::WalkingDown, 
            AnimationFrame::new(1, 0, 2, 0.3, true));
        
        // Right and left walking - using the same 2-frame walking animation
        animations.insert(AnimationState::WalkingLeft, 
            AnimationFrame::new(1, 0, 2, 0.3, true)); // Will be flipped horizontally
        animations.insert(AnimationState::WalkingRight, 
            AnimationFrame::new(1, 0, 2, 0.3, true)); // Normal direction
        
        // Diagonal walking - using the same walking animation
        animations.insert(AnimationState::WalkingUpLeft, 
            AnimationFrame::new(1, 0, 2, 0.3, true)); // Will be flipped
        animations.insert(AnimationState::WalkingUpRight, 
            AnimationFrame::new(1, 0, 2, 0.3, true)); // Normal
        animations.insert(AnimationState::WalkingDownLeft, 
            AnimationFrame::new(1, 0, 2, 0.3, true)); // Will be flipped
        animations.insert(AnimationState::WalkingDownRight, 
            AnimationFrame::new(1, 0, 2, 0.3, true)); // Normal
        
        // Chopping animations (using idle for now, but different durations)
        animations.insert(AnimationState::ChoppingUp, 
            AnimationFrame::new(0, 0, 3, 0.3, false)); // Don't loop action animations
        animations.insert(AnimationState::ChoppingDown, 
            AnimationFrame::new(0, 0, 3, 0.3, false));
        animations.insert(AnimationState::ChoppingLeft, 
            AnimationFrame::new(0, 0, 3, 0.3, false));
        animations.insert(AnimationState::ChoppingRight, 
            AnimationFrame::new(0, 0, 3, 0.3, false));
        
        // Digging animations (using idle for now)
        animations.insert(AnimationState::DiggingUp, 
            AnimationFrame::new(0, 0, 3, 0.4, false));
        animations.insert(AnimationState::DiggingDown, 
            AnimationFrame::new(0, 0, 3, 0.4, false));
        animations.insert(AnimationState::DiggingLeft, 
            AnimationFrame::new(0, 0, 3, 0.4, false));
        animations.insert(AnimationState::DiggingRight, 
            AnimationFrame::new(0, 0, 3, 0.4, false));
        
        // Sword swinging (using idle for now)
        animations.insert(AnimationState::SwingingSwordUp, 
            AnimationFrame::new(0, 0, 3, 0.15, false)); // Fast sword swings
        animations.insert(AnimationState::SwingingSwordDown, 
            AnimationFrame::new(0, 0, 3, 0.15, false));
        animations.insert(AnimationState::SwingingSwordLeft, 
            AnimationFrame::new(0, 0, 3, 0.15, false));
        animations.insert(AnimationState::SwingingSwordRight, 
            AnimationFrame::new(0, 0, 3, 0.15, false));
        
        Self {
            current_state: AnimationState::Idle,
            current_frame: 0,
            frame_timer: 0.0,
            animations,
            frame_width,
            frame_height,
            facing_direction: Vector2::new(0.0, 1.0), // Default facing down
        }
    }
    
    pub fn set_animation(&mut self, new_state: AnimationState) {
        if self.current_state != new_state {
            self.current_state = new_state;
            self.current_frame = 0;
            self.frame_timer = 0.0;
        }
    }
    
    pub fn update(&mut self, delta_time: f32) {
        if let Some(animation) = self.animations.get(&self.current_state) {
            self.frame_timer += delta_time;
            
            if self.frame_timer >= animation.frame_duration {
                self.frame_timer = 0.0;
                self.current_frame += 1;
                
                if self.current_frame >= animation.frame_count {
                    if animation.loop_animation {
                        self.current_frame = 0;
                    } else {
                        // Animation finished, return to idle
                        self.current_frame = animation.frame_count - 1;
                        if self.current_state != AnimationState::Idle {
                            self.set_animation(AnimationState::Idle);
                        }
                    }
                }
            }
        }
    }
    
    pub fn get_current_source_rect(&self) -> Rectangle {
        if let Some(animation) = self.animations.get(&self.current_state) {
            let frame_x = (animation.start_frame + self.current_frame) * self.frame_width;
            let frame_y = animation.row * self.frame_height;
            
            Rectangle::new(
                frame_x as f32,
                frame_y as f32,
                self.frame_width as f32,
                self.frame_height as f32,
            )
        } else {
            // Fallback to first frame of idle
            Rectangle::new(0.0, 0.0, self.frame_width as f32, self.frame_height as f32)
        }
    }
    
    pub fn is_animation_finished(&self) -> bool {
        if let Some(animation) = self.animations.get(&self.current_state) {
            !animation.loop_animation && self.current_frame >= animation.frame_count - 1
        } else {
            true
        }
    }
    
    // Helper function to determine animation based on movement and facing direction
    pub fn get_movement_animation(&self, movement: Vector2) -> AnimationState {
        if movement.length() < 0.1 {
            return AnimationState::Idle;
        }
        
        // Determine direction based on movement vector
        let normalized = movement.normalized();
        
        // Check for diagonal movement first (more specific)
        if normalized.x > 0.5 && normalized.y < -0.5 {
            AnimationState::WalkingUpRight
        } else if normalized.x < -0.5 && normalized.y < -0.5 {
            AnimationState::WalkingUpLeft
        } else if normalized.x > 0.5 && normalized.y > 0.5 {
            AnimationState::WalkingDownRight
        } else if normalized.x < -0.5 && normalized.y > 0.5 {
            AnimationState::WalkingDownLeft
        }
        // Then check cardinal directions
        else if normalized.y < -0.5 {
            AnimationState::WalkingUp
        } else if normalized.y > 0.5 {
            AnimationState::WalkingDown
        } else if normalized.x < -0.5 {
            AnimationState::WalkingLeft
        } else if normalized.x > 0.5 {
            AnimationState::WalkingRight
        } else {
            AnimationState::Idle
        }
    }
    
    // Get action animation based on facing direction
    pub fn get_action_animation(&self, action_type: ActionType) -> AnimationState {
        let dir = self.facing_direction.normalized();
        
        match action_type {
            ActionType::Chopping => {
                if dir.y < -0.5 { AnimationState::ChoppingUp }
                else if dir.y > 0.5 { AnimationState::ChoppingDown }
                else if dir.x < 0.0 { AnimationState::ChoppingLeft }
                else { AnimationState::ChoppingRight }
            }
            ActionType::Digging => {
                if dir.y < -0.5 { AnimationState::DiggingUp }
                else if dir.y > 0.5 { AnimationState::DiggingDown }
                else if dir.x < 0.0 { AnimationState::DiggingLeft }
                else { AnimationState::DiggingRight }
            }
            ActionType::SwingSword => {
                if dir.y < -0.5 { AnimationState::SwingingSwordUp }
                else if dir.y > 0.5 { AnimationState::SwingingSwordDown }
                else if dir.x < 0.0 { AnimationState::SwingingSwordLeft }
                else { AnimationState::SwingingSwordRight }
            }
        }
    }
    
    pub fn set_facing_direction(&mut self, direction: Vector2) {
        if direction.length() > 0.1 {
            self.facing_direction = direction.normalized();
        }
    }
}

#[derive(Clone, Copy, Debug)]
pub enum ActionType {
    Chopping,
    Digging,
    SwingSword,
}