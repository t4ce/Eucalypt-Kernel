SYSROOT := ${SYSROOT} ninja -C build install

CC      := x86_64-elf-gcc
CFLAGS  := -Wall -Wextra -O2 -g
LDFLAGS := --sysroot=$(SYSROOT)

SRC_DIR := .
BIN_DIR := bin
OBJ_DIR := obj
TARGET  := $(BIN_DIR)/main

C_SOURCES := $(shell find $(SRC_DIR) -name '*.c')
OBJECTS   := $(patsubst ./%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) --sysroot=$(SYSROOT) -c $< -o $@

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)