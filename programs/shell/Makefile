BUILD_DIR=./build
FILES=$(BUILD_DIR)/shell.o
INCLUDES= -I../stdlib/src
FLAGS= -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc
all: $(BUILD_DIR) ${FILES}
	i686-elf-gcc -g -T ./linker.ld -o ./shell.elf -ffreestanding -O0 -nostdlib -fpic -g ${FILES} ../stdlib/build/libvana.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/shell.o: ./src/shell.c
	i686-elf-gcc ${INCLUDES} -I./ $(FLAGS) -std=gnu99 -c ./src/shell.c -o $(BUILD_DIR)/shell.o

clean:
	rm -rf ${FILES}
	rm ./shell.elf
.PHONY: all clean
