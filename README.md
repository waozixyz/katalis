# Katalis

A Minecraft-style voxel game built with the Kryon framework and Raylib, featuring procedural terrain generation, first-person controls, and advanced rendering techniques.

## Features

### Voxel Engine
- **Procedural Terrain Generation** - Infinite worlds using Perlin noise with caves and natural features
- **Greedy Meshing** - Optimized rendering with 46% vertex reduction
- **Chunk System** - Efficient world management with dynamic loading/unloading
- **10 Block Types** - Grass, dirt, stone, wood, leaves, sand, water, cobblestone, and bedrock

### Rendering
- **Texture Atlas** - Procedurally generated 256x256 atlas with per-block textures
- **Directional Lighting** - Realistic shading based on face orientation
- **View Distance** - Configurable chunk streaming (default: 8 chunks)
- **Optimized Meshes** - GPU-accelerated rendering with vertex colors

### Player Controls
- **First-Person Camera** - Smooth mouse look with proper FPS controls
- **WASD Movement** - Walk, sprint, and jump mechanics
- **Physics** - Gravity and collision detection with blocks
- **Flying Mode** - Toggle creative-style flying (F key)

### Game Mechanics
- **Collision Detection** - AABB collision preventing movement through solid blocks
- **Ground Detection** - Proper standing/jumping mechanics
- **Camera Controls** - Adjustable sensitivity and view angles

## Controls

| Key | Action |
|-----|--------|
| **W/A/S/D** | Move forward/left/back/right |
| **Mouse** | Look around |
| **Space** | Jump (walking) / Move up (flying) |
| **Left Ctrl** | Move down (flying mode only) |
| **Left Shift** | Sprint (2x speed) |
| **F** | Toggle flying/walking mode |
| **ESC** | Toggle cursor lock |

## Building

### Prerequisites
- Nix package manager (for development environment)
- Kryon framework
- Raylib 5.5
- GCC compiler

### Build Instructions

```bash
# Enter nix development shell
nix-shell

# Build with Make
make

# Run the game
./main

# Or use Kryon CLI
kryon run src/main.c
```

## Project Structure

```
katalis/
├── include/voxel/          # Header files
│   ├── block.h            # Block types and properties
│   ├── chunk.h            # Chunk data structure
│   ├── world.h            # World management
│   ├── noise.h            # Perlin noise generator
│   ├── terrain.h          # Terrain generation
│   ├── player.h           # Player controller
│   └── texture_atlas.h    # Texture atlas system
├── src/
│   ├── main.c             # Entry point with Kryon UI
│   ├── game.c             # Game logic (init/update/draw)
│   └── voxel/             # Voxel engine implementation
│       ├── block.c        # Block system
│       ├── chunk.c        # Chunk meshing (greedy algorithm)
│       ├── world.c        # World with hash map chunks
│       ├── noise.c        # Perlin noise (Ken Perlin's 2002 algorithm)
│       ├── terrain.c      # Procedural terrain with caves
│       ├── player.c       # First-person controller
│       └── texture_atlas.c # Procedural texture generation
├── Makefile               # Build system
└── README.md              # This file
```

## Technical Details

### Voxel Rendering Pipeline
1. **Terrain Generation** - Perlin noise creates heightmaps with caves
2. **Chunk Meshing** - Greedy algorithm merges adjacent faces
3. **Texture Mapping** - Per-face UVs from texture atlas
4. **Lighting** - Vertex colors based on face orientation
5. **GPU Upload** - Optimized mesh with all attributes

### Performance
- **Greedy Meshing**: 46.8% vertex reduction (5,256 → 2,796 vertices)
- **Chunk Rendering**: ~48,000 vertices for 49 chunks
- **Terrain Generation**: 2-3 seconds for 7×7 chunk grid
- **Frame Rate**: Stable 60 FPS with view distance 8

### Algorithms Used
- **Perlin Noise** - Ken Perlin's improved noise (2002)
- **Fractional Brownian Motion** - Multi-octave noise for natural terrain
- **Greedy Meshing** - Face merging optimization
- **AABB Collision** - Axis-aligned bounding box detection
- **FNV-1a Hashing** - Chunk coordinate hashing

## Game Phases Implemented

### ✅ Phase 1: Foundation
- Block system with properties
- Chunk data structure (16×16×256)
- Basic face culling

### ✅ Phase 2: Greedy Meshing
- Advanced mesh optimization
- 46% vertex reduction

### ✅ Phase 3: World Management
- Hash map chunk storage
- Dynamic chunk loading/unloading
- Multi-chunk rendering

### ✅ Phase 4: Procedural Terrain
- Perlin noise implementation
- Heightmap generation
- Cave systems

### ✅ Phase 5: First-Person Camera
- Mouse look controls
- WASD movement
- Flying mode

### ✅ Phase 6: Voxel Renderer
- Texture atlas system
- Directional lighting
- Vertex colors

### ✅ Phase 7: Physics & Collision
- Gravity system
- Block collision detection
- Player physics

## World Generation

The world uses **Perlin noise** with the following parameters:
- **Height Scale**: 24 blocks
- **Sea Level**: Y=32
- **Caves**: Volumetric noise (35% threshold)
- **Layers**: Grass (1 block) → Dirt (4 blocks) → Stone (infinite)

Each world has a unique seed for reproducible terrain.

## License

0BSD - See LICENSE file

## Credits

Built with:
- **Kryon Framework** - UI and rendering backend
- **Raylib 5.5** - 3D graphics library
- **Perlin Noise** - Ken Perlin's improved algorithm
