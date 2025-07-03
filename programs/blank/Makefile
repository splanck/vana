BUILD_DIR=./build
FILES=$(BUILD_DIR)/blank.o
INCLUDES= -I../libc/include -I../stdlib/src
FLAGS= -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc
all: $(BUILD_DIR) ${FILES}
	i686-elf-gcc -g -T ./linker.ld -o ./blank.elf -ffreestanding -O0 -nostdlib -fpic -g ${FILES} ../libc/build/libc.a ../stdlib/build/libvana.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/blank.o: ./blank.c
	i686-elf-gcc ${INCLUDES} -I./ $(FLAGS) -std=gnu99 -c ./blank.c -o $(BUILD_DIR)/blank.o

clean:
	rm -rf ${FILES}
	rm ./blank.elf
.PHONY: all clean
