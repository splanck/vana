BUILD_DIR=./build
INCLUDES=-I./include
FLAGS=-g -ffreestanding -fno-builtin -Wall -O0 -nostdlib -nostartfiles -nodefaultlibs

FILES=\
$(BUILD_DIR)/startup/crt0.o \
$(BUILD_DIR)/sys_wrappers.o \
$(BUILD_DIR)/heap/malloc.o \
$(BUILD_DIR)/heap/free.o \
$(BUILD_DIR)/heap/realloc.o \
$(BUILD_DIR)/heap/calloc.o \
$(BUILD_DIR)/string/memset.o \
$(BUILD_DIR)/string/memcpy.o \
$(BUILD_DIR)/string/strcmp.o \
$(BUILD_DIR)/string/strlen.o

all: $(BUILD_DIR) $(FILES)
	i686-elf-ld -m elf_i386 -relocatable $(FILES) -o $(BUILD_DIR)/libc.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/heap $(BUILD_DIR)/string $(BUILD_DIR)/startup

$(BUILD_DIR)/startup/%.o: startup/%.s | $(BUILD_DIR)
	i686-elf-gcc $(FLAGS) -c $< -o $@ -Wa,--32

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu99 -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
