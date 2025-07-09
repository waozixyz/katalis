use raylib::prelude::*;
use raylib::math::Vector2;
use crate::types::*;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum AnimalType {
    WildChicken,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum AnimalState {
    Idle,
    Wandering,
    Fleeing,
    Dead,
}

#[derive(Clone, Debug)]
pub struct Animal {
    pub animal_type: AnimalType,
    pub position: Vector2,
    pub state: AnimalState,
    pub health: f32,
    pub max_health: f32,
    pub speed: f32,
    pub wander_target: Option<Vector2>,
    pub flee_target: Option<Vector2>,
    pub state_timer: f32,
    pub egg_timer: f32,
    pub last_egg_time: f32,
    pub size: f32,
    pub direction: f32, // Rotation angle in degrees
    pub animation_timer: f32,
    pub is_alive: bool,
    pub despawn_timer: f32,
}

impl Animal {
    pub fn new(animal_type: AnimalType, position: Vector2) -> Self {
        let (max_health, speed, size) = match animal_type {
            AnimalType::WildChicken => (20.0, 60.0, 16.0),
        };
        
        Self {
            animal_type,
            position,
            state: AnimalState::Idle,
            health: max_health,
            max_health,
            speed,
            wander_target: None,
            flee_target: None,
            state_timer: 0.0,
            egg_timer: 0.0,
            last_egg_time: 0.0,
            size,
            direction: 0.0,
            animation_timer: 0.0,
            is_alive: true,
            despawn_timer: 0.0,
        }
    }
    
    pub fn update(&mut self, delta_time: f32, player_pos: Vector2) {
        if !self.is_alive {
            self.despawn_timer += delta_time;
            return;
        }
        
        self.animation_timer += delta_time;
        self.state_timer += delta_time;
        
        // Check if player is nearby and flee if so
        let distance_to_player = self.position.distance_to(player_pos);
        if distance_to_player < 80.0 && self.state != AnimalState::Fleeing {
            self.start_fleeing(player_pos);
        }
        
        match self.state {
            AnimalState::Idle => {
                if self.state_timer > 3.0 {
                    self.start_wandering();
                }
            }
            AnimalState::Wandering => {
                if let Some(target) = self.wander_target {
                    let direction = (target - self.position).normalized();
                    self.position += direction * self.speed * delta_time;
                    
                    // Update visual direction
                    self.direction = direction.x.atan2(direction.y).to_degrees();
                    
                    // Check if reached target
                    if self.position.distance_to(target) < 10.0 {
                        self.state = AnimalState::Idle;
                        self.wander_target = None;
                        self.state_timer = 0.0;
                    }
                }
                
                // Stop wandering after some time
                if self.state_timer > 5.0 {
                    self.state = AnimalState::Idle;
                    self.wander_target = None;
                    self.state_timer = 0.0;
                }
            }
            AnimalState::Fleeing => {
                if let Some(target) = self.flee_target {
                    let direction = (target - self.position).normalized();
                    self.position += direction * self.speed * 2.0 * delta_time; // Flee faster
                    
                    // Update visual direction
                    self.direction = direction.x.atan2(direction.y).to_degrees();
                    
                    // Check if reached safe distance
                    if self.position.distance_to(player_pos) > 150.0 || self.state_timer > 3.0 {
                        self.state = AnimalState::Idle;
                        self.flee_target = None;
                        self.state_timer = 0.0;
                    }
                }
            }
            AnimalState::Dead => {
                // Dead animals don't move
            }
        }
        
        // Egg laying for chickens
        if self.animal_type == AnimalType::WildChicken && self.is_alive {
            self.egg_timer += delta_time;
            if self.egg_timer > 30.0 { // Lay egg every 30 seconds
                self.egg_timer = 0.0;
                self.last_egg_time = 0.0;
            }
        }
    }
    
    fn start_wandering(&mut self) {
        self.state = AnimalState::Wandering;
        self.state_timer = 0.0;
        
        // Pick a random nearby target
        let angle = rand::random::<f32>() * 2.0 * std::f32::consts::PI;
        let distance = 50.0 + rand::random::<f32>() * 100.0;
        let offset = Vector2::new(angle.cos() * distance, angle.sin() * distance);
        self.wander_target = Some(self.position + offset);
    }
    
    fn start_fleeing(&mut self, player_pos: Vector2) {
        self.state = AnimalState::Fleeing;
        self.state_timer = 0.0;
        
        // Flee in opposite direction from player
        let direction = (self.position - player_pos).normalized();
        let flee_distance = 120.0;
        self.flee_target = Some(self.position + direction * flee_distance);
    }
    
    pub fn can_lay_egg(&self) -> bool {
        self.animal_type == AnimalType::WildChicken && 
        self.is_alive && 
        self.state != AnimalState::Fleeing &&
        self.egg_timer >= 30.0
    }
    
    pub fn lay_egg(&mut self) -> bool {
        if self.can_lay_egg() {
            self.egg_timer = 0.0;
            self.last_egg_time = 0.0;
            return true;
        }
        false
    }
    
    pub fn take_damage(&mut self, damage: f32) -> bool {
        if !self.is_alive {
            return false;
        }
        
        self.health -= damage;
        if self.health <= 0.0 {
            self.health = 0.0;
            self.is_alive = false;
            self.state = AnimalState::Dead;
            return true; // Animal died
        }
        false
    }
    
    pub fn can_be_killed(&self) -> bool {
        self.is_alive
    }
    
    pub fn get_drops(&self) -> Vec<(ResourceType, u32)> {
        if !self.is_alive {
            match self.animal_type {
                AnimalType::WildChicken => vec![
                    (ResourceType::RawChicken, 1),
                    (ResourceType::ChickenFeathers, 2 + (rand::random::<u32>() % 3)), // 2-4 feathers
                ],
            }
        } else {
            vec![]
        }
    }
    
    pub fn should_despawn(&self) -> bool {
        !self.is_alive && self.despawn_timer > 10.0 // Despawn after 10 seconds
    }
    
    pub fn get_collision_rect(&self) -> Rectangle {
        Rectangle::new(
            self.position.x - self.size / 2.0,
            self.position.y - self.size / 2.0,
            self.size,
            self.size
        )
    }
    
    pub fn collides_with_point(&self, point: Vector2) -> bool {
        self.position.distance_to(point) < self.size / 2.0
    }
    
    pub fn get_name(&self) -> &'static str {
        match self.animal_type {
            AnimalType::WildChicken => "Wild Chicken",
        }
    }
    
    pub fn get_color(&self) -> Color {
        match self.animal_type {
            AnimalType::WildChicken => {
                if self.is_alive {
                    Color::new(255, 255, 255, 255) // White chicken
                } else {
                    Color::new(160, 160, 160, 255) // Gray when dead
                }
            }
        }
    }
}

#[derive(Clone, Debug)]
pub struct DroppedEgg {
    pub position: Vector2,
    pub despawn_timer: f32,
}

impl DroppedEgg {
    pub fn new(position: Vector2) -> Self {
        Self {
            position,
            despawn_timer: 0.0,
        }
    }
    
    pub fn update(&mut self, delta_time: f32) {
        self.despawn_timer += delta_time;
    }
    
    pub fn should_despawn(&self) -> bool {
        self.despawn_timer > 60.0 // Despawn after 60 seconds
    }
    
    pub fn get_collision_rect(&self) -> Rectangle {
        Rectangle::new(
            self.position.x - 8.0,
            self.position.y - 8.0,
            16.0,
            16.0
        )
    }
    
    pub fn collides_with_point(&self, point: Vector2) -> bool {
        self.position.distance_to(point) < 16.0
    }
}

pub struct AnimalManager {
    pub animals: Vec<Animal>,
    pub dropped_eggs: Vec<DroppedEgg>,
    pub spawn_timer: f32,
}

impl AnimalManager {
    pub fn new() -> Self {
        Self {
            animals: Vec::new(),
            dropped_eggs: Vec::new(),
            spawn_timer: 0.0,
        }
    }
    
    pub fn update(&mut self, delta_time: f32, player_pos: Vector2, world_width: usize, world_height: usize) -> Vec<(ResourceType, u32)> {
        let collected_items = Vec::new();
        
        self.spawn_timer += delta_time;
        
        // Spawn new animals occasionally
        if self.spawn_timer > 20.0 && self.animals.len() < 15 { // Max 15 animals
            self.spawn_timer = 0.0;
            self.spawn_random_animal(world_width, world_height, player_pos);
        }
        
        // Update all animals
        for animal in &mut self.animals {
            animal.update(delta_time, player_pos);
            
            // Check if animal lays egg
            if animal.can_lay_egg() {
                if animal.lay_egg() {
                    self.dropped_eggs.push(DroppedEgg::new(animal.position));
                }
            }
        }
        
        // Update dropped eggs
        for egg in &mut self.dropped_eggs {
            egg.update(delta_time);
        }
        
        // Remove despawned animals and eggs
        self.animals.retain(|animal| !animal.should_despawn());
        self.dropped_eggs.retain(|egg| !egg.should_despawn());
        
        collected_items
    }
    
    fn spawn_random_animal(&mut self, world_width: usize, world_height: usize, player_pos: Vector2) {
        // Spawn away from player
        let mut spawn_pos;
        let mut attempts = 0;
        
        loop {
            let x = rand::random::<f32>() * (world_width as f32 * TILE_SIZE as f32);
            let y = rand::random::<f32>() * (world_height as f32 * TILE_SIZE as f32);
            spawn_pos = Vector2::new(x, y);
            
            if spawn_pos.distance_to(player_pos) > 200.0 || attempts > 10 {
                break;
            }
            attempts += 1;
        }
        
        // Only spawn chickens for now
        let animal = Animal::new(AnimalType::WildChicken, spawn_pos);
        self.animals.push(animal);
    }
    
    pub fn try_collect_egg(&mut self, target_pos: Vector2) -> Option<(ResourceType, u32)> {
        for (i, egg) in self.dropped_eggs.iter().enumerate() {
            if egg.collides_with_point(target_pos) {
                self.dropped_eggs.remove(i);
                return Some((ResourceType::Egg, 1));
            }
        }
        None
    }
    
    pub fn try_kill_animal(&mut self, target_pos: Vector2) -> Option<Vec<(ResourceType, u32)>> {
        for animal in &mut self.animals {
            if animal.collides_with_point(target_pos) && animal.can_be_killed() {
                animal.take_damage(animal.max_health); // Kill instantly
                return Some(animal.get_drops());
            }
        }
        None
    }
    
    pub fn get_animals_in_bounds(&self, min_bound: Vector2, max_bound: Vector2) -> Vec<&Animal> {
        self.animals.iter()
            .filter(|animal| {
                animal.position.x >= min_bound.x && animal.position.x <= max_bound.x &&
                animal.position.y >= min_bound.y && animal.position.y <= max_bound.y
            })
            .collect()
    }
    
    pub fn get_eggs_in_bounds(&self, min_bound: Vector2, max_bound: Vector2) -> Vec<&DroppedEgg> {
        self.dropped_eggs.iter()
            .filter(|egg| {
                egg.position.x >= min_bound.x && egg.position.x <= max_bound.x &&
                egg.position.y >= min_bound.y && egg.position.y <= max_bound.y
            })
            .collect()
    }
}

impl AnimalType {
    pub fn get_name(&self) -> &'static str {
        match self {
            AnimalType::WildChicken => "Wild Chicken",
        }
    }
    
    pub fn get_max_health(&self) -> f32 {
        match self {
            AnimalType::WildChicken => 20.0,
        }
    }
    
    pub fn get_speed(&self) -> f32 {
        match self {
            AnimalType::WildChicken => 60.0,
        }
    }
    
    pub fn get_size(&self) -> f32 {
        match self {
            AnimalType::WildChicken => 16.0,
        }
    }
}