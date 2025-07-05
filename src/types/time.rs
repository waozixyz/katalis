#[derive(Clone, Debug)]
pub struct GameTime {
    pub total_seconds: f32,
    pub time_scale: f32,
}

impl GameTime {
    pub fn new() -> Self {
        Self {
            total_seconds: 43200.0, // Start at noon (12:00)
            time_scale: 60.0, // 1 real second = 1 game minute
        }
    }
    
    pub fn update(&mut self, delta_time: f32) {
        self.total_seconds += delta_time * self.time_scale;
        self.total_seconds = self.total_seconds % 86400.0;
    }
    
    pub fn get_hours(&self) -> u32 {
        (self.total_seconds / 3600.0) as u32
    }
    
    pub fn get_minutes(&self) -> u32 {
        ((self.total_seconds % 3600.0) / 60.0) as u32
    }
    
    pub fn get_time_string(&self) -> String {
        format!("{:02}:{:02}", self.get_hours(), self.get_minutes())
    }
    
    pub fn is_day(&self) -> bool {
        let hour = self.get_hours();
        hour >= 6 && hour < 18
    }
    
    pub fn get_light_level(&self) -> f32 {
        let hour = self.get_hours() as f32 + (self.get_minutes() as f32 / 60.0);
        
        if hour >= 7.0 && hour <= 17.0 {
            1.0
        } else if hour >= 19.0 || hour <= 5.0 {
            0.3
        } else {
            if hour > 17.0 && hour < 19.0 {
                1.0 - (hour - 17.0) / 2.0 * 0.7
            } else {
                0.3 + (hour - 5.0) / 2.0 * 0.7
            }
        }
    }
}