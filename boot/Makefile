boot:
	mkdir -p out/boot
	gcc -c boot/hello.s -o out/boot/hello.o
	objdump --reloc -d out/boot/hello.o > out/boot/hello.objdump
	boot/obj2bl.py out/boot/hello.objdump out/boot/hello.bl
	qemu-system-x86_64 -hda out/boot/hello.bl

.PHONY: boot
