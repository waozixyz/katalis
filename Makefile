# Makefile for Katalis (Direct Raylib - Kryon Removed)

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

# Project includes
INCLUDES = -Iinclude -Isrc

# Raylib and SDL3 flags
SDL3_FLAGS = $(shell pkg-config --cflags --libs sdl3 2>/dev/null || echo "-lSDL3")
RAYLIB_FLAGS = $(shell pkg-config --cflags --libs raylib 2>/dev/null || echo "-lraylib")

# Linker libraries (keep what's needed for game features)
LIBS = $(SDL3_FLAGS) $(RAYLIB_FLAGS) -lSDL3_ttf -lharfbuzz -lfreetype -lfribidi -lGL -lm -pthread

# Source files
TARGET = main

# Voxel engine sources (organized by module)

# Core module
VOXEL_CORE = src/voxel/core/block.c \
             src/voxel/core/item.c \
             src/voxel/core/texture_atlas.c

# World module
VOXEL_WORLD = src/voxel/world/world.c \
              src/voxel/world/chunk.c \
              src/voxel/world/chunk_worker.c \
              src/voxel/world/terrain.c \
              src/voxel/world/noise.c \
              src/voxel/world/biome.c \
              src/voxel/world/spawn.c \
              src/voxel/world/water.c \
              src/voxel/world/chest.c \
              src/voxel/world/raycast.c

# Entity module
VOXEL_ENTITY = src/voxel/entity/entity.c \
               src/voxel/entity/entity_utils.c \
               src/voxel/entity/collision.c \
               src/voxel/entity/pig.c \
               src/voxel/entity/sheep.c \
               src/voxel/entity/tree.c \
               src/voxel/entity/block_human.c

# Player module
VOXEL_PLAYER = src/voxel/player/player.c

# Inventory module
VOXEL_INVENTORY = src/voxel/inventory/inventory.c \
                  src/voxel/inventory/inventory_input.c \
                  src/voxel/inventory/inventory_ui.c \
                  src/voxel/inventory/crafting.c

# UI module
VOXEL_UI = src/voxel/ui/pause_menu.c \
           src/voxel/ui/minimap.c \
           src/voxel/ui/settings_menu.c

# Render module
VOXEL_RENDER = src/voxel/render/sky.c \
               src/voxel/render/light.c \
               src/voxel/render/particle.c \
               src/voxel/render/chunk_batcher.c

# Network module
VOXEL_NETWORK = src/voxel/network/network.c \
                src/voxel/network/serialization.c

# Combined
VOXEL_SOURCES = $(VOXEL_CORE) $(VOXEL_WORLD) $(VOXEL_ENTITY) $(VOXEL_PLAYER) \
                $(VOXEL_INVENTORY) $(VOXEL_UI) $(VOXEL_RENDER) $(VOXEL_NETWORK)

APP_SOURCES = src/main.c src/game.c $(VOXEL_SOURCES)

.PHONY: all clean run

all: $(TARGET)

main: $(APP_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $(APP_SOURCES) $(LIBS) -o $@

clean:
	rm -f $(TARGET) *.kir

run: main
	./main
