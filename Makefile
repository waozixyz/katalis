# Makefile for Katalis (Kryon C Project)

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
KRYON_PATH = /mnt/storage/Projects/kryon
INCLUDES = -I$(KRYON_PATH)/bindings/c -I$(KRYON_PATH)/ir -I$(KRYON_PATH)/backends/desktop -I$(KRYON_PATH)/ir/third_party/cJSON
SDL3_FLAGS = $(shell pkg-config --cflags --libs sdl3 2>/dev/null || echo "-lSDL3")
RAYLIB_FLAGS = $(shell pkg-config --cflags --libs raylib 2>/dev/null || echo "-lraylib")
LDFLAGS = -L$(KRYON_PATH)/build
LIBS = -lkryon_desktop -lkryon_ir $(SDL3_FLAGS) $(RAYLIB_FLAGS) -lm

# Kryon C source files (compile directly since library doesn't build)
KRYON_C_DIR = $(KRYON_PATH)/bindings/c
KRYON_SOURCES = $(KRYON_C_DIR)/kryon.c $(KRYON_C_DIR)/kryon_dsl.c

# Source files
TARGET = main

# Voxel engine sources
VOXEL_SOURCES = src/voxel/block.c \
                src/voxel/chunk.c \
                src/voxel/world.c \
                src/voxel/noise.c \
                src/voxel/terrain.c \
                src/voxel/player.c \
                src/voxel/texture_atlas.c \
                src/voxel/item.c \
                src/voxel/inventory.c \
                src/voxel/inventory_ui.c \
                src/voxel/inventory_input.c \
                src/voxel/crafting.c \
                src/voxel/pause_menu.c \
                src/voxel/entity.c \
                src/voxel/block_human.c \
                src/voxel/sky.c \
                src/voxel/tree.c \
                src/voxel/network.c \
                src/voxel/minimap.c \
                src/voxel/light.c

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
