pub mod resources;
pub mod time;
pub mod trees;
pub mod terrain;
pub mod tools; // NEW
pub mod animals;

// Re-export commonly used types
pub use resources::*;
pub use time::*;
pub use trees::*;
pub use terrain::*;
pub use tools::*; // NEW
pub use animals::*;

// Keep shared constants here
pub const TILE_SIZE: i32 = 32;
pub const PLAYER_SPEED: f32 = 200.0;
pub const CAMERA_SMOOTHNESS: f32 = 8.0;
pub const LASER_SPEED: f32 = 800.0;
pub const LASER_MAX_DISTANCE: f32 = 400.0;
pub const TREE_BURN_TIME: f32 = 3.0;
pub const STUMP_FADE_TIME: f32 = 30.0;
pub const MINING_RANGE: f32 = 64.0; // NEW: Mining range
pub const MINING_TIME: f32 = 2.0; // NEW: Time to mine a resource