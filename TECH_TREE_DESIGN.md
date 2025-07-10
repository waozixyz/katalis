# Katalis Tech Tree Design

## Overview
A branching tech tree system where players progress through different technology paths simultaneously, unlocking new capabilities and eventually advancing to the next age.

## Tech Tree Structure

### Age 1: Stone Age
Players start here and must complete several tech branches to advance.

#### **Survival Branch**
- **Basic Tools** → **Advanced Stone Tools** → **Tool Mastery**
- **Shelter** → **Fire Control** → **Basic Cooking**

#### **Food Branch** 
- **Hunting** → **Food Preparation** → **Food Storage**

#### **Materials Branch**
- **Stone Working** → **Basic Crafting** → **Material Processing**

### Age 2: Early Civilization  
Requires: Complete all Age 1 branches

#### **Metallurgy Branch**
- **Copper Working** → **Bronze Making** → **Advanced Alloys**

#### **Agriculture Branch**
- **Plant Gathering** → **Basic Farming** → **Crop Processing**

#### **Construction Branch**
- **Basic Buildings** → **Specialized Structures** → **Infrastructure**

### Age 3: Iron Age
Requires: Complete all Age 2 branches + specific prerequisites

#### **Iron Working Branch**
- **Iron Smelting** → **Steel Making** → **Advanced Metallurgy**

#### **Advanced Agriculture Branch**
- **Grain Processing** → **Bread Making** → **Food Systems**

#### **Engineering Branch**
- **Simple Machines** → **Complex Structures** → **Mechanical Systems**

### Age 4: Industrial Age
Requires: Bread Making + Iron Working + Engineering

#### **Steam Power Branch**
- **Steam Basics** → **Steam Machinery** → **Power Distribution**

#### **Automation Branch**
- **Basic Automation** → **Complex Automation** → **Mass Production**

#### **Electrical Branch**
- **Basic Electronics** → **Power Systems** → **Advanced Electronics**

## Tech Box System

### Box Types
1. **Unlocked & Incomplete** - Yellow border, shows progress
2. **Unlocked & Complete** - Green border with checkmark
3. **Locked** - Gray border, shows prerequisites
4. **Available** - Blue border, can be started

### Box Contents
Each tech box contains:
- **Title** (e.g., "Basic Tools")
- **Progress Bar** (completed/total objectives)
- **Icon** representing the tech
- **Hover Details**:
  - Specific objectives with progress
  - Prerequisites if locked
  - Unlocked recipes/buildings

### Current Objectives Overlay
- Shows 3-4 current available tech boxes
- Positioned at top-right, stays visible during gameplay
- Clicking opens full tech tree
- Shows mini progress bars

## Specific Tech Trees

### Age 1 Detailed Breakdown

#### **Basic Tools** Box
**Prerequisites**: None (starting tech)
**Objectives**:
- Collect 20 Rocks
- Collect 15 Sticks  
- Craft Stone Pickaxe
- Craft Stone Axe

**Unlocks**: Stone tool recipes

#### **Fire Control** Box
**Prerequisites**: Basic Tools
**Objectives**:
- Build Campfire
- Build Charcoal Pit
- Make 5 Charcoal

**Unlocks**: Fire-based recipes, cooking

#### **Hunting** Box
**Prerequisites**: Basic Tools  
**Objectives**:
- Kill 3 Chickens
- Collect 5 Raw Chicken
- Craft Stone Sword

**Unlocks**: Combat mechanics, hunting

#### **Food Preparation** Box
**Prerequisites**: Fire Control + Hunting
**Objectives**:
- Cook 3 Chickens on Campfire
- Collect 10 Plant Fiber
- Store 10 Cooked Food

**Unlocks**: Basic food systems

#### **Stone Working** Box
**Prerequisites**: Basic Tools
**Objectives**:
- Collect 50 Stone
- Build Stone Anvil
- Craft 10 Stone tools total

**Unlocks**: Advanced stone crafting

#### **Material Processing** Box
**Prerequisites**: Stone Working + Fire Control
**Objectives**:
- Process 20 Wood into Planks
- Create 10 Wooden Components
- Build Workshop Storage

**Unlocks**: Complex crafting chains

### Age 2 Prerequisites
To advance to Age 2, players must complete:
- All 6 Age 1 tech boxes
- Minimum resource stockpiles
- At least 3 different building types

### UI Layout

#### **Full Tech Tree View** (T key)
```
[Age 1: Stone Age]
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ Basic Tools │───▶│Fire Control │───▶│ Food Prep   │
│   ✓ 4/4     │    │   ⚠ 2/3     │    │   🔒 0/3    │
└─────────────┘    └─────────────┘    └─────────────┘
                   
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  Hunting    │───▶│Stone Working│───▶│Mat Process  │
│   ⚠ 1/3     │    │   ⚠ 2/3     │    │   🔒 0/4    │
└─────────────┘    └─────────────┘    └─────────────┘

[Age 2: Early Civilization] 🔒
Requires: Complete all Age 1 boxes
```

#### **Current Objectives Overlay**
```
┌─ Current Objectives ─────────────────┐
│ 🔥 Fire Control        [██▒▒] 2/3     │
│ ⚔️  Hunting           [█▒▒▒] 1/3     │  
│ 🔨 Stone Working      [██▒▒] 2/3     │
│                                      │
│ Press T for full tech tree           │
└──────────────────────────────────────┘
```

## Hotkey Controls
- **T**: Open/close full tech tree view
- **Hover**: Show detailed objectives
- **Click**: Focus on specific tech branch
- **ESC**: Close tech tree

## Integration Points
- **Crafting**: Recipes locked until tech unlocked
- **Building**: Structures locked until tech unlocked  
- **Resources**: Some resources require tech to gather efficiently
- **UI**: Tech overlay stays visible during all gameplay
- **Progression**: Age advancement requires completing tech trees

## Benefits of This System
1. **Clear Goals**: Players always know what to work on next
2. **Choice**: Multiple paths to progress simultaneously  
3. **Guidance**: Hover details provide exact steps
4. **Visual Progress**: Easy to see advancement
5. **Flexibility**: Can work on multiple branches
6. **Discovery**: Unlocking reveals new possibilities