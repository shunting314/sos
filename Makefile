CC=i686-elf-gcc
LD=i686-elf-ld
OBJCOPY=i686-elf-objcopy

run: image
	# the options to setup monitor by telnet is copied from https://stackoverflow.com/questions/49716931/how-to-run-qemu-with-nographic-and-monitor-but-still-be-able-to-send-ctrlc-to
	qemu-system-x86_64 -nographic -monitor telnet::1234,server,nowait -hda out/kernel.img

# run qemu in graphic mode
run-graph: image
	qemu-system-x86_64 -hda out/kernel.img

image: kernel bootloader
	kernel/check_kernel_size
	cat out/boot/bootloader.bl out/kernel/kernel > out/kernel.img
	truncate -s `printf "%d\n" 0x50200` out/kernel.img

kernel: kernel/entry.s kernel/kernel.ld
	mkdir -p out/kernel
	$(CC) -c kernel/entry.s -o out/kernel/entry.o
	$(LD) out/kernel/entry.o -T kernel/kernel.ld -o out/kernel/kernel

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
