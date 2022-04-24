CC := i686-elf-gcc
CXX := i686-elf-g++
LD := i686-elf-ld
OBJCOPY := i686-elf-objcopy
OBJDUMP := i686-elf-objdump

LIB_SRC_C := $(wildcard lib/*.c)
LIB_OBJ := $(patsubst %.c,%.o,$(LIB_SRC_C))
LIB_OBJ := $(addprefix out/,$(LIB_OBJ)) # add out/ prefix

KERNEL_SRC_CPP := $(wildcard kernel/*.cpp)
KERNEL_SRC_C := $(wildcard kernel/*.c)
KERNEL_SRC_S := $(wildcard kernel/*.s) $(wildcard kernel/*.S)
KERNEL_OBJ := $(patsubst %.cpp,%.o,$(KERNEL_SRC_CPP)) $(patsubst %.c,%.o,$(KERNEL_SRC_C)) $(patsubst %.s,%.o,$(KERNEL_SRC_S))
KERNEL_OBJ := $(addprefix out/,$(KERNEL_OBJ)) # add out/ prefix

ALL_OBJ := $(LIB_OBJ) $(KERNEL_OBJ)
# the list contains .d file for .s file which does not exist. But it's fine since -include
# ignore missing files anyway.
ALL_DEPS := $(patsubst %.o,%.d,$(ALL_OBJ))

# -fno-builtin-printf is added so gcc does not try to use puts to optimize printf.
CFLAGS := -MD -I. -Iinc -fno-builtin-printf -Werror -Wno-builtin-declaration-mismatch $(EXTRA_CFLAGS)

# set run as the default rule
run:

out/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	
out/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

# add this rule explicitly since the pattern rule will treat the .S file
# as .s file which causes errors.
out/kernel/interrupt_entry.o: kernel/interrupt_entry.S
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@

out/%.o: %.s
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@

out/kernel/kernel: kernel/kernel.ld $(KERNEL_OBJ) $(LIB_OBJ)
	mkdir -p $(dir $@)
	$(LD) $(KERNEL_OBJ) $(LIB_OBJ) -T kernel/kernel.ld -o $@
	$(OBJDUMP) -d $@ > out/kernel/kernel.asm

out/boot/bootloader.bl: out/boot/bootloader.o out/boot/load_kernel_and_enter.o
	mkdir -p $(dir $@)
	$(LD) $^ -Ttext 0x7c00 -e entry -o out/boot/bootloader.elf
	$(OBJCOPY) -O binary out/boot/bootloader.elf out/boot/bootloader
	boot/pad_boot_loader

img out/kernel.img: out/kernel/kernel out/boot/bootloader.bl
	kernel/check_kernel_size
	cat out/boot/bootloader.bl out/kernel/kernel > out/kernel.img
	truncate -s `printf "%d\n" 0x50200` out/kernel.img

# We put the filesystem in hdb rather than put it together with the kernel in hda
# to make it easy to recreate kernel image while keeping the fs image.
run: out/kernel.img out/fs.img
	# the options to setup monitor by telnet is copied from https://stackoverflow.com/questions/49716931/how-to-run-qemu-with-nographic-and-monitor-but-still-be-able-to-send-ctrlc-to
	# Using -nographic option does not show content written to the vga memory.
	# Use -curses works. Check https://stackoverflow.com/questions/6710555/how-to-use-qemu-to-run-a-non-gui-os-on-the-terminal for details.
	# simulate 10M memory
	qemu-system-x86_64 -curses -monitor telnet::2000,server,nowait -hda out/kernel.img -hdb out/fs.img -m 10 -no-reboot -no-shutdown $(QEMU_EXTRA)

# start a connection to qemu monitor
mon:
	nc localhost 2000

# run qemu in graphic mode
run-graph: out/kernel.img out/fs.img
	qemu-system-x86_64 -hda out/kernel.img -hdb out/fs.img

# will not recreate if out/fs.img already exists. This makes sure we don't drop mutations in the filesystem.
out/fs.img:
	mkdir -p out/fs_template
	echo "Hello, simfs!" > out/fs_template/message
	python3.6 mkfs.py out/fs_template out/fs.img $(MKFS_EXTRA) # python points to python2 in make's shell instance but point to python3.6 outside of make. I have to explicitly specify python3.6 for now since mkfs.py requires python3. TODO: figure out the root cause

clean:
	rm -rf out/

# this rule use mac compiler. obj2bl.py does the trick to do relocation.
bootloader_helloworld:
	mkdir -p out/boot
	gcc -c boot/hello.s -o out/boot/hello.o
	objdump --reloc -d out/boot/hello.o > out/boot/hello.objdump
	VERBOSE=1 boot/obj2bl.py out/boot/hello.objdump out/boot/hello.bl
	qemu-system-x86_64 -hda out/boot/hello.bl

-include $(ALL_DEPS)
