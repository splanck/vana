ARCH ?= i686

ifeq ($(ARCH),x86_64)
CROSS_PREFIX = x86_64-elf
NASM_FORMAT = elf64
else
CROSS_PREFIX = i686-elf
NASM_FORMAT = elf
endif

CC = $(CROSS_PREFIX)-gcc
LD = $(CROSS_PREFIX)-ld

KERNEL_CFLAGS = -m64 -mcmodel=kernel -fno-pic -fno-pie -mno-red-zone -ffreestanding -fno-builtin -nostdlib -mno-mmx -mno-sse
LDFLAGS = -z max-page-size=0x1000 -T src/linker64.ld

DISK_OBJS = ./build/disk/disk.o \
            ./build/disk/streamer.o

KEYBOARD_OBJS = ./build/keyboard/keyboard.o \
                ./build/keyboard/classic.o
TASK_OBJS = ./build/task/task.o \
            ./build/task/process.o \
            ./build/task/task.asm.o
LOADER_OBJS = ./build/loader/formats/elf.o \
              ./build/loader/formats/elfloader.o

FILES = ./build/kernel.asm.o \
        ./build/kernel.o \
        ./build/gdt.o \
        ./build/gdt.asm.o \
        ./build/tss.asm.o \
        ./build/idt.o \
        ./build/idt.asm.o \
        ./build/memory.o \
        ./build/string.o \
        ./build/pic.o \
        ./build/io.o \
        $(DISK_OBJS) \
        $(KEYBOARD_OBJS) \
        $(TASK_OBJS) \
       $(LOADER_OBJS) \
       ./build/isr80h/isr80h.o \
       ./build/isr80h/io.o \
       ./build/isr80h/heap.o \
       ./build/isr80h/misc.o \
       ./build/isr80h/process.o \
       ./build/memory/heap/heap.o \
        ./build/memory/heap/kheap.o \
        ./build/memory/paging/paging.o \
        ./build/memory/paging/paging.asm.o \
        ./build/fs/file.o \
        ./build/fs/pparser.o \
        ./build/fs/fat/fat16.o
INCLUDES = -I./src -I./src/gdt -I./src/task -I./src/idt -I./src/fs -I./src/fs/fat -I./src/loader/formats -I./src/isr80h
BUILD_DIRS = ./bin ./build/memory/heap ./build/memory/paging ./build/keyboard ./build/disk ./build/fs ./build/fs/fat ./build/task ./build/loader/formats ./build/isr80h ./build/boot64 ./build/syscall
FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc -fno-pie -no-pie

# Directory where the FAT image will be mounted
MOUNT_DIR ?= /mnt/d

dirs:
	mkdir -p $(BUILD_DIRS)

user_programs:
	cd ./programs/stdlib && $(MAKE)
	cd ./programs/blank && $(MAKE)
	cd ./programs/shell && $(MAKE)

user_programs_clean:
	- cd ./programs/stdlib && $(MAKE) clean
	- cd ./programs/blank && $(MAKE) clean
	- cd ./programs/shell && $(MAKE) clean

all: user_programs dirs ./bin/boot.bin ./bin/kernel.bin
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os.bin
	# Mount the disk image and copy user programs
	sudo mkdir -p $(MOUNT_DIR)
	sudo mount -t vfat ./bin/os.bin $(MOUNT_DIR)
	sudo cp ./programs/blank/blank.elf $(MOUNT_DIR)
	sudo cp ./programs/shell/shell.elf $(MOUNT_DIR)
	sudo umount $(MOUNT_DIR)

./bin/kernel.bin: $(FILES)
	$(LD) -g -relocatable $(FILES) -o ./build/kernelfull.o
	$(CC) $(FLAGS) -T ./src/linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o

./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

./build/kernel.asm.o: ./src/kernel.asm
	nasm -f $(NASM_FORMAT) -g ./src/kernel.asm -o ./build/kernel.asm.o

./build/kernel.o: ./src/kernel.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o

./build/gdt.o: ./src/gdt/gdt.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/gdt/gdt.c -o ./build/gdt.o

./build/gdt.asm.o: ./src/gdt/gdt.asm
	nasm -f $(NASM_FORMAT) -g ./src/gdt/gdt.asm -o ./build/gdt.asm.o

./build/tss.asm.o: ./src/task/tss.asm
	nasm -f $(NASM_FORMAT) -g ./src/task/tss.asm -o ./build/tss.asm.o

./build/idt.o: ./src/idt/idt.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/idt/idt.c -o ./build/idt.o

./build/idt.asm.o: ./src/idt/idt.asm
	nasm -f $(NASM_FORMAT) -g ./src/idt/idt.asm -o ./build/idt.asm.o

./build/task/task.o: ./src/task/task.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/task/task.c -o ./build/task/task.o

./build/task/process.o: ./src/task/process.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/task/process.c -o ./build/task/process.o

./build/task/task.asm.o: ./src/task/task.asm
	nasm -f $(NASM_FORMAT) -g ./src/task/task.asm -o ./build/task/task.asm.o

./build/memory.o: ./src/memory/memory.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/memory/memory.c -o ./build/memory.o

./build/string.o: ./src/string/string.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/string/string.c -o ./build/string.o

./build/pic.o: ./src/pic/pic.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/pic/pic.c -o ./build/pic.o

./build/io.o: ./src/io/io.asm
	nasm -f $(NASM_FORMAT) -g ./src/io/io.asm -o ./build/io.o

./build/disk/disk.o: ./src/disk/disk.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/disk/disk.c -o ./build/disk/disk.o

./build/disk/streamer.o: ./src/disk/streamer.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/disk/streamer.c -o ./build/disk/streamer.o

./build/keyboard/keyboard.o: ./src/keyboard/keyboard.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/keyboard/keyboard.c -o ./build/keyboard/keyboard.o

./build/keyboard/classic.o: ./src/keyboard/classic.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/keyboard/classic.c -o ./build/keyboard/classic.o

./build/memory/heap/heap.o: ./src/memory/heap/heap.c
	$(CC) $(INCLUDES) -I./src/memory/heap $(FLAGS) -std=gnu99 -c ./src/memory/heap/heap.c -o ./build/memory/heap/heap.o

./build/memory/heap/kheap.o: ./src/memory/heap/kheap.c
	$(CC) $(INCLUDES) -I./src/memory/heap $(FLAGS) -std=gnu99 -c ./src/memory/heap/kheap.c -o ./build/memory/heap/kheap.o

./build/memory/paging/paging.o: ./src/memory/paging/paging.c
	$(CC) $(INCLUDES) -I./src/memory/paging $(FLAGS) -std=gnu99 -c ./src/memory/paging/paging.c -o ./build/memory/paging/paging.o

./build/memory/paging/paging.asm.o: ./src/memory/paging/paging.asm
	nasm -f $(NASM_FORMAT) -g ./src/memory/paging/paging.asm -o ./build/memory/paging/paging.asm.o

./build/fs/file.o: ./src/fs/file.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/fs/file.c -o ./build/fs/file.o

./build/fs/pparser.o: ./src/fs/pparser.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/fs/pparser.c -o ./build/fs/pparser.o

./build/fs/fat/fat16.o: ./src/fs/fat/fat16.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/fs/fat/fat16.c -o ./build/fs/fat/fat16.o

./build/loader/formats/elf.o: ./src/loader/formats/elf.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/loader/formats/elf.c -o ./build/loader/formats/elf.o

./build/loader/formats/elfloader.o: ./src/loader/formats/elfloader.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/loader/formats/elfloader.c -o ./build/loader/formats/elfloader.o

./build/isr80h/isr80h.o: ./src/isr80h/isr80h.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/isr80h/isr80h.c -o ./build/isr80h/isr80h.o

./build/isr80h/io.o: ./src/isr80h/io.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/isr80h/io.c -o ./build/isr80h/io.o

./build/isr80h/heap.o: ./src/isr80h/heap.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/isr80h/heap.c -o ./build/isr80h/heap.o

./build/isr80h/misc.o: ./src/isr80h/misc.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/isr80h/misc.c -o ./build/isr80h/misc.o

./build/isr80h/process.o: ./src/isr80h/process.c
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c ./src/isr80h/process.c -o ./build/isr80h/process.o

clean: user_programs_clean
	rm -rf ./bin/boot.bin
	rm -rf ./bin/kernel.bin
	rm -rf ./bin/os.bin
	rm -rf ${FILES}
	rm -rf ./build/kernelfull.o
.PHONY: run
run:
	qemu-system-i386 -drive format=raw,file=./bin/os.bin

.PHONY: vana64
vana64: dirs bin/boot64.bin bin/kernel64.bin
	rm -rf ./bin/os64.bin
	dd if=./bin/boot64.bin >> ./bin/os64.bin
	dd if=./bin/kernel64.bin >> ./bin/os64.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os64.bin

build/kernel64.o: src/kernel64.c
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

ifeq ($(ARCH),x86_64)
build/kernel64.asm.o: src/kernel64.asm
	nasm -f elf64 -g src/kernel64.asm -o build/kernel64.asm.o

build/memory/paging/paging64.o: src/memory/paging/paging64.c
	$(CC) $(KERNEL_CFLAGS) -I./src -c src/memory/paging/paging64.c -o build/memory/paging/paging64.o

build/gdt/gdt64.o: src/gdt/gdt64.c
	$(CC) $(KERNEL_CFLAGS) -I./src -c src/gdt/gdt64.c -o build/gdt/gdt64.o

build/gdt/gdt64.asm.o: src/gdt/gdt64.asm
	nasm -f elf64 -g src/gdt/gdt64.asm -o build/gdt/gdt64.asm.o

build/idt64.o: src/idt/idt64.c
	$(CC) $(KERNEL_CFLAGS) -I./src -c src/idt/idt64.c -o build/idt64.o

build/idt64.asm.o: src/idt/idt64.asm
	nasm -f elf64 -g src/idt/idt64.asm -o build/idt64.asm.o

build/page_fault.asm.o: src/idt/page_fault.asm
	nasm -f elf64 -g src/idt/page_fault.asm -o build/page_fault.asm.o

build/page_fault.o: src/idt/page_fault.c
	$(CC) $(KERNEL_CFLAGS) -I./src -c src/idt/page_fault.c -o build/page_fault.o

build/task/tss64.asm.o: src/task/tss64.asm
	nasm -f elf64 -g src/task/tss64.asm -o build/task/tss64.asm.o

build/task/process.o: src/task/process.c
	$(CC) $(KERNEL_CFLAGS) -I./src -c src/task/process.c -o build/task/process.o

build/syscall/syscall.o: src/syscall/syscall.c
	$(CC) $(KERNEL_CFLAGS) -I./src -c src/syscall/syscall.c -o build/syscall/syscall.o

build/syscall/syscall.asm.o: src/syscall/syscall.asm
	nasm -f elf64 -g src/syscall/syscall.asm -o build/syscall/syscall.asm.o

bin/kernel64.bin: build/kernel64.asm.o build/kernel64.o build/memory/paging/paging64.o build/gdt/gdt64.asm.o build/gdt/gdt64.o build/idt64.o build/idt64.asm.o build/page_fault.o build/page_fault.asm.o build/task/tss64.asm.o build/task/process.o build/syscall/syscall.o build/syscall/syscall.asm.o
	$(LD) $(LDFLAGS) build/kernel64.asm.o build/kernel64.o build/memory/paging/paging64.o build/gdt/gdt64.asm.o build/gdt/gdt64.o build/idt64.o build/idt64.asm.o build/page_fault.o build/page_fault.asm.o build/task/tss64.asm.o build/task/process.o build/syscall/syscall.o build/syscall/syscall.asm.o -o bin/kernel64.bin
endif

build/boot64/boot.o: src/boot64/boot.asm
	nasm -f elf32 -g src/boot64/boot.asm -o build/boot64/boot.o

bin/boot64.bin: build/boot64/boot.o
	$(LD) -Ttext 0x7c00 build/boot64/boot.o -o build/boot64/boot.elf
	objcopy -O binary build/boot64/boot.elf bin/boot64.bin
