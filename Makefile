INC_DIR := include

# Compiler and flags
CC := gcc
CFLAGS := -Wall -std=c99 -O2 -I$(INC_DIR) -Ibuild/external/raylib/src -I/usr/include/SDL2 -DPLATFORM_DESKTOP_SDL
LDFLAGS := -lSDL2 -lm -ldl -lpthread -lGL -lrt -lX11

# Paths
SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin
RAYLIB_DIR := build/external/raylib
RAYLIB_LIB := $(RAYLIB_DIR)/src/libraylib.a

# Sources and objects
SRCS := $(wildcard $(SRC_DIR)/*.c) $(wildcard $(INC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Output executable
TARGET := $(BIN_DIR)/game

.PHONY: all clean directories

all: directories $(TARGET)

# Link, using explicit path to libraylib.a (no -lraylib)
$(TARGET): $(OBJS) 
	$(CC) $^ -o $@ $(RAYLIB_LIB) $(LDFLAGS)

# Compile .c to .o in build/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

# Create build and bin dirs if missing
directories:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)/*

