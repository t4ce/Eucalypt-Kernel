CC := gcc

CFLAGS := -Wall -Wextra -O2 -ffreestanding -fno-stack-protector -g
CPPFLAGS :=
LDFLAGS :=

SRC_DIR := .
BIN_DIR := bin
OBJ_DIR := obj

TARGET := $(BIN_DIR)/main

C_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJECTS := $(patsubst ./%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) \
		$(LDFLAGS) \
		-nostdlib \
		-Ttext=0x400000 \
		-o $@ \
		$(OBJECTS)
	cp $@ ./main

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		-c $< \
		-o $@

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR) main