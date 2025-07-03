BUILD_DIR=./build
STRING_SRC=$(wildcard string/*.c)
HEAP_SRC=$(wildcard heap/*.c)
OBJS=$(patsubst string/%.c,$(BUILD_DIR)/%.o,$(STRING_SRC)) \
     $(patsubst heap/%.c,$(BUILD_DIR)/%.o,$(HEAP_SRC))
INCLUDES=-I./include
CFLAGS=-g -ffreestanding -fno-builtin -Wall -O0 -nostdlib

all: $(BUILD_DIR) $(BUILD_DIR)/libc.a

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/libc.a: $(OBJS)
	i686-elf-ar rcs $@ $(OBJS)

$(BUILD_DIR)/%.o: string/%.c | $(BUILD_DIR)
	i686-elf-gcc $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: heap/%.c | $(BUILD_DIR)
	i686-elf-gcc $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
