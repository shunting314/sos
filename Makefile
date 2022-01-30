CC=i686-elf-gcc
CXX=i686-elf-g++
LD=i686-elf-ld
OBJCOPY=i686-elf-objcopy

# -fno-builtin-printf is added so gcc does not try to use puts to optimize printf.
CFLAGS=-I. -Iinc -fno-builtin-printf

run: image
	# the options to setup monitor by telnet is copied from https://stackoverflow.com/questions/49716931/how-to-run-qemu-with-nographic-and-monitor-but-still-be-able-to-send-ctrlc-to
	# Using -nographic option does not show content written to the vga memory.
	# Use -curses works. Check https://stackoverflow.com/questions/6710555/how-to-use-qemu-to-run-a-non-gui-os-on-the-terminal for details.
	qemu-system-x86_64 -curses -monitor telnet::1234,server,nowait -hda out/kernel.img

# run qemu in graphic mode
run-graph: image
	qemu-system-x86_64 -hda out/kernel.img

image: kernel bootloader
	kernel/check_kernel_size
	cat out/boot/bootloader.bl out/kernel/kernel > out/kernel.img
	truncate -s `printf "%d\n" 0x50200` out/kernel.img

# TODO: clearnup makefile so adding new file becomes easier
kernel: kernel/entry.s kernel/kernel.ld
	mkdir -p out/kernel
	mkdir -p out/lib
	$(CC) -c kernel/entry.s -o out/kernel/entry.o
	$(CC) -c kernel/asm_util.s -o out/kernel/asm_util.o
	$(CC) -c kernel/kernel_main.c $(CFLAGS) -o out/kernel/kernel_main.o
	$(CC) -c kernel/vga.c $(CFLAGS) -o out/kernel/vga.o
	$(CC) -c kernel/stdio.c $(CFLAGS) -o out/kernel/stdio.o
	$(CXX) -c kernel/idt.cpp $(CFLAGS) -o out/kernel/idt.o
	$(CC) -c lib/string.c $(CFLAGS) -o out/lib/string.o
	$(LD) out/kernel/entry.o out/kernel/asm_util.o out/kernel/kernel_main.o out/kernel/vga.o out/kernel/stdio.o out/kernel/idt.o out/lib/string.o -T kernel/kernel.ld -o out/kernel/kernel

bootloader: boot/bootloader.s
	mkdir -p out/boot
	$(CC) -c boot/bootloader.s -o out/boot/bootloader.o
	$(CC) -c boot/load_kernel_and_enter.c -o out/boot/load_kernel_and_enter.o
	$(LD) out/boot/bootloader.o out/boot/load_kernel_and_enter.o -Ttext 0x7c00 -e entry -o out/boot/bootloader.elf
	$(OBJCOPY) -O binary out/boot/bootloader.elf out/boot/bootloader
	boot/pad_boot_loader

# this rule use mac compiler. obj2bl.py does the trick to do relocation.
bootloader_helloworld:
	mkdir -p out/boot
	gcc -c boot/hello.s -o out/boot/hello.o
	objdump --reloc -d out/boot/hello.o > out/boot/hello.objdump
	VERBOSE=1 boot/obj2bl.py out/boot/hello.objdump out/boot/hello.bl
	qemu-system-x86_64 -hda out/boot/hello.bl

.PHONY: bootloader_helloworld kernel
