# Katalis Crafting Guide

A comprehensive guide to the crafting system in Katalis, organized by production chains and complexity.

## 🎮 Quick Start

- **Open Crafting**: Press `E` to open inventory/crafting interface
- **Craft Items**: Click on items in the crafting panel (right side)
- **Requirements**: Green items can be crafted, red items need more resources
- **Queue System**: Items automatically queue if you're already crafting

---

## 📋 Crafting Categories

### 🔧 Basic Materials
Simple processing of raw resources.

### 🪵 Woodworking
Processing wood into planks, beams, and wooden components.

### ⚒️ Metallurgy
Advanced metal processing requiring furnaces and anvils.

### 🟫 Copper Working
Specialized copper processing and alloy creation.

### 🧵 Textiles
Fiber processing for cloth production.

### 🌾 Food Production
Agriculture and food processing for sustenance.

### 🔥 Steam Systems
Steam power generation and steam-powered machinery.

### 🏗️ Structures
Buildings and infrastructure for automation.

### ⚙️ Automation
Advanced systems for factory optimization.

---

## 🏭 Production Chains

### ⚫ Charcoal Production Chain
***
Wood → Charcoal Pit → Charcoal
***
- **Wood** (3) → **Charcoal** (2)
- **Time**: 4.0 seconds
- **Structure**: Charcoal Pit required

### 🪵 Wood Processing Chain
***
Wood → Manual → Wooden Planks (4)
Wooden Planks → Manual → Wooden Beams (2)
Wooden Planks → Manual → Wooden Gears (1)
Wooden Beams + Wooden Planks → Manual → Wooden Frames (1)
Wooden Beams + Metal Rods → Manual → Wooden Rollers (2)
***

### 🔩 Iron Processing Chain
***
Iron Ore + Coal → Bloomery Furnace → Iron Bloom
Iron Bloom → Stone Anvil → Wrought Iron
Wrought Iron → Stone Anvil → Iron Plates (2)
***

**Advanced Iron Products:**
- **Iron Plates** (2) → **Iron Gears** (1) *(Manual)*
- **Iron Plates** (1) → **Metal Rods** (2) *(Manual)*

### 🟫 Copper Processing Chain
***
Copper Ore + Charcoal → Crude Furnace → Copper Ingots
Copper Ingots → Stone Anvil → Copper Plates (3)
Copper Plates → Manual → Copper Wire (4)
***

**Advanced Copper Products:**
- **Copper Plates** (2) + **Iron Plates** (1) → **Bronze Alloy** (2) *(Manual)*
- **Copper Wire** (3) → **Copper Coils** (1) *(Manual)*
- **Copper Plates** (1) → **Copper Pipes** (2) *(Manual)*

### 🧶 Textile Production Chain
***
Cotton → Spinning Wheel → Threads
Threads → Weaving Machine → Fabric
Fabric → Manual → Cloth Strips (4)
***

### 🌾 Wheat & Bread Production Chain
***
Wheat → Windmill → Flour
Flour + Water → Manual → Dough
Dough → Stone Oven → Bread
***

### 🔥 Steam Power Chain
***
Water + Fuel → Steam Boiler → Steam
Steam → Steam Pipes → Steam Distribution Hub
Steam Power → Steam Conveyor/Steam Pump/Steam Hammer
***

### 🚚 Conveyor Belt Evolution Chain
***
Basic Path: Wood → Wooden Planks → Wooden Frames + Cloth → Basic Conveyor Belt
Reinforced Path: Iron/Metal Processing → Reinforced Conveyor
Steam Path: Basic Conveyor + Steam System → Steam Conveyor
Electric Path: Reinforced Conveyor + Electrical → Electric Conveyor
***

---

## 🏗️ Building Recipes

### Basic Structures
| Building | Materials | Time | Description |
|----------|-----------|------|-------------|
| **Charcoal Pit** | Stone (10) + Clay (5) | 5.0s | Converts wood to charcoal |
| **Crude Furnace** | Stone (8) + Clay (6) | 4.0s | Smelts copper ore |
| **Stone Anvil** | Stone (20) | 6.0s | Metal working station |
| **Spinning Wheel** | Wood (25) | 10.0s | Processes cotton into threads |

### Advanced Structures
| Building | Materials | Time | Description |
|----------|-----------|------|-------------|
| **Bloomery Furnace** | Stone (15) + Clay (8) | 8.0s | Smelts iron ore into blooms |
| **Weaving Machine** | Wood (35) | 15.0s | Converts threads to fabric |
| **Advanced Forge** | Stone (25) + Iron Plates (8) + Copper Plates (4) | 20.0s | High-tier metal processing |

### Food Production Structures
| Building | Materials | Time | Description |
|----------|-----------|------|-------------|
| **Windmill** | Wood (40) + Stone (20) + Iron Gears (4) + Cloth Strips (8) | 25.0s | Grinds wheat into flour |
| **Stone Oven** | Stone (30) + Clay (15) + Iron Plates (4) | 15.0s | Bakes bread and other foods |
| **Grain Silo** | Wood (50) + Stone (20) + Iron Plates (6) | 18.0s | Stores large quantities of grain |

### Steam Infrastructure
| Building | Materials | Time | Description |
|----------|-----------|------|-------------|
| **Steam Boiler** | Iron Plates (6) + Copper Pipes (4) + Metal Rods (2) | 15.0s | Converts water + fuel to steam |
| **Steam Distribution Hub** | Iron Plates (4) + Steam Pipes (8) + Pressure Valve (1) | 10.0s | Distributes steam to multiple machines |

---

## ⚙️ Complete Recipe List

### 🔧 Basic Materials
| Item | Inputs | Output | Time | Structure |
|------|--------|--------|------|-----------|
| **Charcoal** | Wood (3) | Charcoal (2) | 4.0s | Charcoal Pit |

### 🪵 Woodworking
| Item | Inputs | Output | Time | Structure |
|------|--------|--------|------|-----------|
| **Wooden Planks** | Wood (2) | Wooden Planks (4) | 2.0s | Manual |
| **Wooden Beams** | Wooden Planks (3) | Wooden Beams (2) | 3.0s | Manual |
| **Wooden Gears** | Wooden Planks (2) | Wooden Gears (1) | 2.5s | Manual |
| **Wooden Frames** | Wooden Beams (2) + Wooden Planks (1) | Wooden Frames (1) | 4.0s | Manual |
| **Wooden Rollers** | Wooden Beams (1) + Metal Rods (1) | Wooden Rollers (2) | 3.0s | Manual |

### ⚒️ Metallurgy
| Item | Inputs | Output | Time | Structure |
|------|--------|--------|------|-----------|
| **Iron Bloom** | Iron Ore (2) + Coal (1) | Iron Bloom (1) | 6.0s | Bloomery Furnace |
| **Wrought Iron** | Iron Bloom (1) | Wrought Iron (1) | 3.0s | Stone Anvil |
| **Iron Plates** | Wrought Iron (1) | Iron Plates (2) | 4.0s | Stone Anvil |
| **Iron Gears** | Iron Plates (2) | Iron Gears (1) | 2.0s | Manual |
| **Metal Rods** | Iron Plates (1) | Metal Rods (2) | 1.5s | Manual |
| **Steel Ingots** | Iron Plates (2) + Charcoal (1) | Steel Ingots (1) | 8.0s | Advanced Forge |
| **Steel Plates** | Steel Ingots (1) | Steel Plates (2) | 5.0s | Advanced Forge |

### 🟫 Copper Working
| Item | Inputs | Output | Time | Structure |
|------|--------|--------|------|-----------|
| **Copper Ingots** | Copper Ore (2) + Charcoal (1) | Copper Ingots (1) | 4.0s | Crude Furnace |
| **Copper Plates** | Copper Ingots (1) | Copper Plates (3) | 3.0s | Stone Anvil |
| **Copper Wire** | Copper Plates (1) | Copper Wire (4) | 2.0s | Manual |
| **Copper Coils** | Copper Wire (3) | Copper Coils (1) | 3.0s | Manual |
| **Copper Pipes** | Copper Plates (1) | Copper Pipes (2) | 2.5s | Manual |
| **Bronze Alloy** | Copper Plates (2) + Iron Plates (1) | Bronze Alloy (2) | 6.0s | Manual |
| **Electrical Components** | Copper Wire (2) + Iron Gears (1) | Electrical Components (1) | 4.0s | Manual |

### 🧵 Textiles
| Item | Inputs | Output | Time | Structure |
|------|--------|--------|------|-----------|
| **Threads** | Cotton (2) | Threads (3) | 3.0s | Spinning Wheel |
| **Fabric** | Threads (3) | Fabric (1) | 4.0s | Weaving Machine |
| **Cloth Strips** | Fabric (1) | Cloth Strips (4) | 1.0s | Manual |
| **Reinforced Fabric** | Fabric (2) + Copper Wire (1) | Reinforced Fabric (1) | 5.0s | Manual |

### 🌾 Food Production
| Item | Inputs | Output | Time | Structure |
|------|--------|--------|------|-----------|
| **Flour** | Wheat (3) | Flour (2) | 5.0s | Windmill |
| **Flour** | Wheat (3) | Flour (2) | 4.0s | Water Mill |
| **Dough** | Flour (2) + Water (1) | Dough (1) | 2.0s | Manual |
| **Bread** | Dough (1) | Bread (2) | 8.0s | Stone Oven |
| **Scythe** | Wood (3) + Metal Rods (2) + Iron Plates (1) | Scythe (1) | 4.0s | Manual |

### 🔥 Steam Systems
| Item | Inputs | Output | Time | Structure |
|------|--------|--------|------|-----------|
| **Water Bucket** | Metal Rods (2) + Iron Plates (1) | Water Bucket (1) | 2.0s | Manual |
| **Steam Pipes** | Copper Pipes (2) + Iron Plates (1) | Steam Pipes (4) | 3.0s | Manual |
| **Pressure Valve** | Iron Plates (2) + Copper Plates (1) + Iron Gears (1) | Pressure Valve (1) | 4.0s | Manual |
| **Steam Boiler** | Iron Plates (6) + Copper Pipes (4) + Metal Rods (2) | Steam Boiler (1) | 15.0s | Manual |
| **Steam Distribution Hub** | Iron Plates (4) + Steam Pipes (8) + Pressure Valve (1) | Steam Distribution Hub (1) | 10.0s | Manual |

### 🏗️ Structures
| Item | Inputs | Output | Time | Structure |
|------|--------|--------|------|-----------|
| **Charcoal Pit** | Stone (10) + Clay (5) | Charcoal Pit (1) | 5.0s | Manual |
| **Crude Furnace** | Stone (8) + Clay (6) | Crude Furnace (1) | 4.0s | Manual |
| **Bloomery Furnace** | Stone (15) + Clay (8) | Bloomery Furnace (1) | 8.0s | Manual |
| **Stone Anvil** | Stone (20) | Stone Anvil (1) | 6.0s | Manual |
| **Spinning Wheel** | Wood (25) | Spinning Wheel (1) | 10.0s | Manual |
| **Weaving Machine** | Wood (35) | Weaving Machine (1) | 15.0s | Manual |
| **Advanced Forge** | Stone (25) + Iron Plates (8) + Copper Plates (4) | Advanced Forge (1) | 20.0s | Manual |
| **Water Pump** | Iron Plates (4) + Copper Pipes (6) + Iron Gears (2) | Water Pump (1) | 12.0s | Manual |
| **Windmill** | Wood (40) + Stone (20) + Iron Gears (4) + Cloth Strips (8) | Windmill (1) | 25.0s | Manual |
| **Water Mill** | Wood (30) + Stone (25) + Iron Gears (6) + Copper Pipes (4) | Water Mill (1) | 20.0s | Manual |
| **Stone Oven** | Stone (30) + Clay (15) + Iron Plates (4) | Stone Oven (1) | 15.0s | Manual |
| **Grain Silo** | Wood (50) + Stone (20) + Iron Plates (6) | Grain Silo (1) | 18.0s | Manual |

### ⚙️ Automation
| Item | Inputs | Output | Time | Structure |
|------|--------|--------|------|-----------|
| **Basic Conveyor Belt** | Wooden Frames (1) + Wooden Rollers (2) + Cloth Strips (3) | Basic Conveyor Belt (1) | 4.0s | Manual |
| **Reinforced Conveyor** | Metal Rods (2) + Iron Plates (1) + Iron Gears (1) + Cloth Strips (2) | Reinforced Conveyor (1) | 5.0s | Manual |
| **Steam Conveyor** | Basic Conveyor Belt (1) + Steam Pipes (2) + Pressure Valve (1) | Steam Conveyor (1) | 6.0s | Manual |
| **Electric Conveyor** | Reinforced Conveyor (1) + Electrical Components (1) + Copper Wire (2) | Electric Conveyor (1) | 8.0s | Manual |
| **Steam Pump** | Water Pump (1) + Steam Pipes (3) + Pressure Valve (1) | Steam Pump (1) | 8.0s | Manual |
| **Steam Hammer** | Stone Anvil (1) + Steam Pipes (4) + Iron Plates (6) + Pressure Valve (2) | Steam Hammer (1) | 25.0s | Manual |
| **Sorting Machine** | Iron Plates (6) + Copper Plates (4) + Electrical Components (2) + Iron Gears (3) | Sorting Machine (1) | 15.0s | Manual |
| **Steam Engine** | Iron Plates (8) + Copper Pipes (6) + Steel Plates (2) + Iron Gears (4) + Pressure Valve (2) | Steam Engine (1) | 20.0s | Manual |
| **Power Cables** | Copper Wire (5) + Reinforced Fabric (1) | Power Cables (3) | 3.0s | Manual |

---

## 🗺️ Resource Requirements

### Primary Resources
- **Wood**: From trees (using Axe)
- **Stone**: From stone quarries (using Pickaxe)
- **Iron Ore**: From iron veins (using Pickaxe)
- **Coal**: From coal deposits (using Pickaxe)
- **Clay**: From clay deposits (using Shovel)
- **Copper Ore**: From copper veins (using Pickaxe)
- **Cotton**: From cotton patches (using Axe)
- **Water**: From rivers/lakes (using Water Bucket)
- **Wheat**: From wheat fields (using Scythe)

### Mining Yields
| Resource | Yield per Action | Tool Required |
|----------|------------------|---------------|
| Wood | 8-15 (varies by tree) | Axe |
| Iron Ore | 3 | Pickaxe |
| Coal | 2 | Pickaxe |
| Stone | 5 | Pickaxe |
| Clay | 4 | Shovel |
| Copper Ore | 4 | Pickaxe |
| Cotton | 3 | Axe |
| Water | 10 | Water Bucket |
| Wheat | 5 | Scythe |

---

## 💡 Production Tips

### Efficiency Guidelines
1. **Build Early**: Construct basic structures (Charcoal Pit, Crude Furnace, Stone Anvil) as soon as possible
2. **Resource Planning**: Mine extra stone and clay early for building construction
3. **Wood Processing**: Set up wooden plank production early for conveyor belts
4. **Food Security**: Establish wheat farming and bread production for sustenance
5. **Dual Metal Production**: Run iron and copper processing in parallel
6. **Queue Management**: Use the crafting queue to batch produce items
7. **Location Strategy**: Place buildings near resource sources to minimize transport

### Optimal Build Order
1. **Charcoal Pit** - Essential for fuel production
2. **Crude Furnace** - Start copper processing early
3. **Stone Anvil** - Required for all metal processing
4. **Spinning Wheel** - Start textile production early
5. **Basic Conveyor Belt** - Early game automation
6. **Windmill/Water Mill** - Food production infrastructure
7. **Stone Oven** - Complete bread production chain
8. **Bloomery Furnace** - Advanced iron processing
9. **Weaving Machine** - Complete textile chain
10. **Steam Boiler** - Mid-game power source
11. **Steam Distribution Hub** - Steam network management
12. **Advanced Forge** - High-tier metallurgy
13. **Steam-Powered Machines** - Automated production
14. **Electric Systems** - Late-game factory optimization

### Resource Ratios
- **Wood Processing**: 2 Wood → 4 Wooden Planks → 2 Wooden Beams (1.33:1 efficiency)
- **Iron Processing**: 2 Iron Ore + 1 Coal → 1 Iron Bloom → 1 Wrought Iron → 2 Iron Plates
- **Copper Processing**: 2 Copper Ore + 1 Charcoal → 1 Copper Ingot → 3 Copper Plates
- **Textile Chain**: 2 Cotton → 3 Threads → 1 Fabric → 4 Cloth Strips
- **Bread Production**: 3 Wheat → 2 Flour → 1 Dough → 2 Bread (6:4 wheat to bread ratio)
- **Fuel Production**: 3 Wood → 2 Charcoal (net loss but higher energy density)
- **Steam System**: 1 Charcoal/Coal → 10 Steam Units (continuous operation)
- **Conveyor Progression**: Basic → Steam (1.5x speed) → Electric (2x speed)

### Steam System Operation
- **Steam Boiler**: Requires constant fuel (Charcoal/Coal) and water input
- **Steam Pressure**: Maintained automatically, but requires Pressure Valves for safety
- **Steam Distribution**: Uses Steam Pipes to connect boiler to powered machines
- **Fuel Consumption**: 1 Charcoal or Coal per 10 steam units produced
- **Water Consumption**: 1 Water unit per 5 steam units produced

### Food Production Management
- **Wheat Farming**: Establish multiple wheat fields for consistent harvests
- **Mill Placement**: Position Windmills on hills for better wind, Water Mills near rivers
- **Bread Storage**: Use Grain Silos for bulk wheat storage before processing
- **Production Balance**: Maintain 3:2:1 ratio of wheat fields to mills to ovens

---

## 🔧 Building Placement

### Structure Requirements
- **Size Variations**: Buildings range from 1x1 (Conveyor) to 4x4 (Steam Engine)
- **Terrain**: Most buildings require grass, stone, or sand terrain
- **Clearance**: Remove trees and avoid resource veins when placing buildings
- **Accessibility**: Ensure you can reach building interfaces for manual item transfer
- **Steam Planning**: Consider steam pipe routing for steam-powered machines
- **Power Planning**: Consider future electrical connections for advanced automation

### Building Sizes
| Building | Size | Special Notes |
|----------|------|---------------|
| Basic Conveyor Belt | 1x1 | Slowest but cheapest transport |
| Steam Conveyor | 1x1 | Requires steam connection |
| Electric Conveyor | 1x1 | Requires power connection |
| Charcoal Pit | 2x2 | Animated when active |
| Crude Furnace | 2x2 | Animated, copper-focused |
| Stone Anvil | 2x2 | Static structure |
| Spinning Wheel | 2x2 | Static structure |
| Water Pump | 2x2 | Requires water source nearby |
| Steam Pump | 2x2 | Automated water collection |
| Steam Distribution Hub | 2x2 | Central steam routing |
| Stone Oven | 2x2 | Requires fuel for baking |
| Weaving Machine | 3x2 | Rectangular footprint |
| Bloomery Furnace | 3x3 | Large structure, animated |
| Advanced Forge | 3x3 | High-tier processing |
| Steam Boiler | 3x3 | Requires fuel and water input |
| Steam Hammer | 3x3 | Automated metal processing |
| Sorting Machine | 3x3 | Automation hub |
| Windmill | 3x3 | Requires open area for wind |
| Water Mill | 3x3 | Requires water source nearby |
| Grain Silo | 3x3 | Vertical storage structure |
| Steam Engine | 4x4 | Largest structure, power generation |

---

## ⚡ Advanced Strategies

### Conveyor Belt Progression
- **Basic Conveyor**: Wood + cloth construction, slow but early-game accessible
- **Steam Conveyor**: Upgrade path using steam power, 1.5x speed boost
- **Reinforced Conveyor**: Metal-based construction, more durable
- **Electric Conveyor**: End-game option, fastest transport speed

### Multi-Metal Production
- **Parallel Processing**: Run iron and copper chains simultaneously
- **Alloy Production**: Combine metals for stronger materials (Bronze, Steel)
- **Resource Balancing**: Copper is more abundant but iron is more versatile
- **Steam Integration**: Use Steam Hammers for automated metal processing

### Food Production Systems
- **Mill Selection**: Water Mills are faster but require water access, Windmills work anywhere
- **Seasonal Planning**: Plant wheat fields in cycles for continuous harvesting
- **Storage Management**: Use Grain Silos to buffer between harvest and processing
- **Automation Integration**: Use conveyors to transport wheat, flour, and bread

### Steam Power Systems
- **Boiler Placement**: Position near water sources and fuel supplies
- **Steam Networks**: Plan pipe layouts to minimize steam loss
- **Pressure Management**: Install Pressure Valves at key distribution points
- **Fuel Automation**: Use Steam Conveyors to automatically feed boilers
- **Power Scaling**: Multiple boilers can supply larger factory networks

### Electrical Systems
- **Power Generation**: Steam engines convert steam to electricity
- **Wiring Networks**: Plan copper wire production for electrical infrastructure
- **Automation Scaling**: Electric conveyors are faster than steam alternatives
- **Mixed Power**: Combine steam and electric systems for optimal efficiency

### Late Game Optimization
- **Hybrid Conveyor Networks**: Mix Basic, Steam, and Electric conveyors by purpose
- **Multi-Building Setups**: Run multiple furnaces/anvils in parallel
- **Input/Output Management**: Use building inventories efficiently
- **Production Balancing**: Match input and output rates across production chains
- **Steam-Electric Integration**: Use steam for heavy machinery, electricity for transport
- **Material Flow**: Design efficient transportation networks with sorting machines

### Factory Layout Tips
- **Furnace Clusters**: Group Crude Furnaces and Bloomery Furnaces together
- **Central Processing**: Place Stone Anvil and Advanced Forge centrally
- **Steam Distribution**: Create central steam hubs with multiple connection points
- **Food Production Zones**: Keep wheat fields, mills, and ovens in dedicated areas
- **Resource Separation**: Keep iron and copper processing areas distinct but connected
- **Transportation Planning**: Design conveyor networks before building production
- **Future Expansion**: Leave space for steam pipes, power cables, and advanced automation
- **Water Access**: Plan factory layouts around water sources for steam systems and water mills

### Production Efficiency Tips
- **Early Steam Setup**: Build steam infrastructure as soon as metal processing allows
- **Food Security**: Establish bread production early for worker efficiency
- **Fuel Management**: Maintain steady charcoal production for continuous steam
- **Water Logistics**: Use Steam Pumps to automate water collection
- **Conveyor Optimization**: Use faster conveyors for high-throughput production lines
- **Steam Pressure**: Monitor steam distribution to prevent bottlenecks
- **Automation Priorities**: Automate resource transport before production processes

---

## 🎯 Progression Milestones

### Early Game (Wood & Stone Age)
- [ ] Build Charcoal Pit
- [ ] Establish basic wood processing (Planks → Beams)
- [ ] Create first Basic Conveyor Belt
- [ ] Set up Crude Furnace and Stone Anvil
- [ ] Craft Scythe for wheat harvesting

### Early-Mid Game (Agricultural Age)
- [ ] Establish wheat farming
- [ ] Build Windmill or Water Mill
- [ ] Construct Stone Oven
- [ ] Complete bread production chain
- [ ] Build Grain Silo for storage

### Mid Game (Steam Age)
- [ ] Construct Steam Boiler system
- [ ] Build Steam Distribution Hub
- [ ] Upgrade to Steam Conveyors
- [ ] Implement Steam Hammer for automation

### Late Game (Industrial Age)
- [ ] Establish electrical power generation
- [ ] Build comprehensive conveyor networks
- [ ] Implement full factory automation
- [ ] Optimize production with sorting systems

### End Game (Optimization)
- [ ] Multi-factory production networks
- [ ] Advanced material processing
- [ ] Complete automation systems
- [ ] Efficient resource management networks
- [ ] Integrated food and industrial production