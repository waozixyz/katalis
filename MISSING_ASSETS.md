# Missing Assets for Chicken/Animal System

This file lists all the assets that need to be created for the chicken/animal system implementation. These are PNG files that should be placed in the appropriate directories.

## Animal Resources Icons
These should be 32x32 pixel PNG files in the `assets/icons/` directory:

- `assets/icons/egg.png` - Icon for chicken eggs
- `assets/icons/raw_chicken.png` - Icon for raw chicken meat
- `assets/icons/cooked_chicken.png` - Icon for cooked chicken meat
- `assets/icons/chicken_feathers.png` - Icon for chicken feathers

## Animal Sprites
These should be sprite sheets or individual sprites for the game world:

- `assets/animals/wild_chicken.png` - Sprite sheet for wild chickens (multiple frames for animation)
  - Suggested size: 32x32 pixels per frame
  - Frames needed: idle, walking, fleeing
  - Could be a single static image initially

- `assets/animals/wild_chicken_dead.png` - Sprite for dead chickens
  - Suggested size: 32x32 pixels
  - Could be a darker/grayed version of the main sprite

- `assets/items/egg_dropped.png` - Sprite for eggs dropped in the world
  - Suggested size: 16x16 pixels
  - Should be visible against grass and other terrain

## Optional Enhancement Assets
These could be added later for improved visuals:

- `assets/animals/chicken_shadow.png` - Shadow sprite for chickens
- `assets/effects/chicken_flee_dust.png` - Dust effect when chickens flee
- `assets/effects/feather_particle.png` - Feather particles when chicken is killed
- `assets/sounds/chicken_cluck.wav` - Sound effect for chicken interactions
- `assets/sounds/chicken_death.wav` - Sound effect when chicken dies
- `assets/sounds/egg_collect.wav` - Sound effect for collecting eggs

## Placeholder Implementation

For now, the game will use simple colored rectangles as placeholders:
- Wild Chickens: White rectangles (16x16 pixels)
- Dead Chickens: Gray rectangles (16x16 pixels)  
- Dropped Eggs: Light yellow rectangles (8x8 pixels)

The icon system will fall back to solid colored squares if the PNG files are not found.

## Asset Creation Notes

When creating these assets:
1. Use a consistent art style that matches the existing game assets
2. Keep the color palette simple and readable
3. Ensure good contrast against the game's terrain tiles
4. Consider different animation frames for walking/idle states
5. Make sure the assets are properly sized for the game's zoom level

## Installation

To use these assets:
1. Create the PNG files according to the specifications above
2. Place them in the correct directories as specified
3. The game will automatically load them when present
4. No code changes are needed - the file paths are already configured

## Future Expansions

This system is designed to be extensible. Future animal types can be added by:
1. Adding new animal types to the `AnimalType` enum
2. Creating corresponding asset files
3. Adding any new resource types to the `ResourceType` enum
4. The rendering and interaction systems will automatically handle new animal types