# Makefile for Katalis (Kryon C Project)

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
KRYON_PATH = /mnt/storage/Projects/kryon
INCLUDES = -I$(KRYON_PATH)/bindings/c -I$(KRYON_PATH)/ir -I$(KRYON_PATH)/backends/desktop -I$(KRYON_PATH)/ir/third_party/cJSON
SDL3_FLAGS = $(shell pkg-config --cflags --libs sdl3 2>/dev/null || echo "-lSDL3")
RAYLIB_FLAGS = $(shell pkg-config --cflags --libs raylib 2>/dev/null || echo "-lraylib")
LDFLAGS = -L$(KRYON_PATH)/build -Wl,-rpath,$(KRYON_PATH)/build
LIBS = -lkryon_desktop -lkryon_ir $(SDL3_FLAGS) $(RAYLIB_FLAGS) -lm -pthread

# Kryon C source files (compile directly since library doesn't build)
KRYON_C_DIR = $(KRYON_PATH)/bindings/c
KRYON_SOURCES = $(KRYON_C_DIR)/kryon.c $(KRYON_C_DIR)/kryon_dsl.c

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
              src/voxel/world/spawn.c

# Entity module
VOXEL_ENTITY = src/voxel/entity/entity.c \
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
           src/voxel/ui/minimap.c

# Render module
VOXEL_RENDER = src/voxel/render/sky.c \
               src/voxel/render/light.c

# Network module
VOXEL_NETWORK = src/voxel/network/network.c

# Combined
VOXEL_SOURCES = $(VOXEL_CORE) $(VOXEL_WORLD) $(VOXEL_ENTITY) $(VOXEL_PLAYER) \
                $(VOXEL_INVENTORY) $(VOXEL_UI) $(VOXEL_RENDER) $(VOXEL_NETWORK)

APP_SOURCES = src/main.c src/game.c $(VOXEL_SOURCES)

.PHONY: all clean run

all: $(TARGET)

main: $(APP_SOURCES) $(KRYON_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -Iinclude -Isrc $(APP_SOURCES) $(KRYON_C_DIR)/kryon.c $(KRYON_C_DIR)/kryon_dsl.c $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -f $(TARGET) *.kir

run: main
	./main
	@if [ -f output.kir ]; then \
		echo "✓ Generated output.kir successfully!"; \
		ls -lh output.kir; \
	fi
