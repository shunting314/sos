bootloader:
	mkdir -p out/boot
	gcc -c boot/bootloader.s -o out/boot/bootloader.o
	objdump --reloc -d out/boot/bootloader.o > out/boot/bootloader.objdump
	VERBOSE=1 boot/obj2bl.py out/boot/bootloader.objdump out/boot/bootloader.bl
	qemu-system-x86_64 -hda out/boot/bootloader.bl

bootloader_helloworld:
	mkdir -p out/boot
	gcc -c boot/hello.s -o out/boot/hello.o
	objdump --reloc -d out/boot/hello.o > out/boot/hello.objdump
	VERBOSE=1 boot/obj2bl.py out/boot/hello.objdump out/boot/hello.bl
	qemu-system-x86_64 -hda out/boot/hello.bl

.PHONY: bootloader_helloworld
