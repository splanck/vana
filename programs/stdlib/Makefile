SRC_DIR=./src
BUILD_DIR=./build

FILES=$(BUILD_DIR)/vana.asm.o \
      $(BUILD_DIR)/vana.o \
      $(BUILD_DIR)/stdlib.o \
      $(BUILD_DIR)/stdio.o \
      $(BUILD_DIR)/string.o \
      $(BUILD_DIR)/memory.o

INCLUDES=-I$(SRC_DIR)
FLAGS=-g -ffreestanding -fno-builtin -Wall -O0 -nostdlib -nostartfiles -nodefaultlibs

all: $(FILES)
	i686-elf-ld -m elf_i386 -relocatable $(FILES) -o $(BUILD_DIR)/libvana.o

$(BUILD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	nasm -f elf $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	i686-elf-gcc $(INCLUDES) $(FLAGS) -std=gnu99 -c $< -o $@

clean:
	rm -f $(BUILD_DIR)/*.o

.PHONY: all clean
