CC := i686-elf-gcc
CXX := i686-elf-g++
LD := i686-elf-ld
OBJCOPY := i686-elf-objcopy
OBJDUMP := i686-elf-objdump
STRIP := i686-elf-strip

CLIB_SRC_C := $(wildcard clib/*.c)
CLIB_SRC_CPP := $(wildcard clib/*.cpp)
CLIB_OBJ := $(patsubst %.c,%.o,$(CLIB_SRC_C)) $(patsubst %.cpp,%.o,$(CLIB_SRC_CPP))
CLIB_OBJ := $(addprefix out/,$(CLIB_OBJ)) # add out/ prefix

ULIB_SRC_CPP := $(wildcard ulib/*.cpp)
ULIB_SRC_S := $(wildcard ulib/*.s)
ULIB_OBJ := $(patsubst %.cpp,%.o,$(ULIB_SRC_CPP)) $(patsubst %.s,%.o,$(ULIB_SRC_S))
ULIB_OBJ := $(addprefix out/,$(ULIB_OBJ)) # add out/ prefix

KERNEL_SRC_CPP := $(wildcard kernel/*.cpp) $(wildcard kernel/nic/*.cpp) $(wildcard kernel/net/*.cpp) $(wildcard kernel/usb/*.cpp)
KERNEL_SRC_C := $(wildcard kernel/*.c)
KERNEL_SRC_S := $(wildcard kernel/*.s) $(wildcard kernel/*.S)
KERNEL_OBJ := $(patsubst %.cpp,%.o,$(KERNEL_SRC_CPP)) $(patsubst %.c,%.o,$(KERNEL_SRC_C)) $(patsubst %.s,%.o,$(KERNEL_SRC_S))
KERNEL_OBJ := $(addprefix out/,$(KERNEL_OBJ)) # add out/ prefix

ALL_OBJ := $(CLIB_OBJ) $(ULIB_OBJ) $(KERNEL_OBJ)
# the list contains .d file for .s file which does not exist. But it's fine since -include
# ignore missing files anyway.
ALL_DEPS := $(patsubst %.o,%.d,$(ALL_OBJ))

USB_BOOT := 1

# -fno-builtin-printf is added so gcc does not try to use puts to optimize printf.
# -Wno-pointer-arith allows arithmetic using void * assuming element size to be 1.
#    This is only needed for C++. C compiler does not warn this by default.
CFLAGS := -MD -I. -Icinc -fno-builtin-printf -Werror -Wno-builtin-declaration-mismatch -Wno-pointer-arith -g -DUSB_BOOT=$(USB_BOOT) $(EXTRA_CFLAGS)

# extra CFLAGS for boot loader
BOOT_CFLAGS := -Os

USER_CFLAGS := $(CFLAGS) -Iuinc

# set run as the default rule
run:

# run with a fresh fs image
crun clean-run:
	rm -f fs.img
	$(MAKE) clean
	$(MAKE) run

# specialize for c file under boot to add bootloader specific CFLAGS
out/boot/%.o: boot/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(BOOT_CFLAGS) -c $< -o $@

out/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compiler cpp file for ulib
# Put this before the more general 'out/%.o' pattern rule so this specialized one
# takes effect rather than the more general one
out/ulib/%.o: ulib/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(USER_CFLAGS) -c $< -o $@

out/user/%.o: user/%.cpp
	mkdir -p out/user
	$(CC) $(USER_CFLAGS) -c $< -o $@

out/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

# add this rule explicitly since the pattern rule will treat the .S file
# as .s file which causes errors.
out/kernel/interrupt_entry.o: kernel/interrupt_entry.S
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@

# specialize for .s file under boot to add bootloader specific CFLAGS
out/boot/%.o: boot/%.s
	mkdir -p $(dir $@)
	$(CC) -c $(BOOT_CFLAGS) $< -o $@

out/%.o: %.s
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@

out/kernel/kernel: kernel/kernel.ld $(KERNEL_OBJ) $(CLIB_OBJ)
	mkdir -p $(dir $@)
	$(LD) $(KERNEL_OBJ) $(CLIB_OBJ) -T kernel/kernel.ld -o out/kernel/kernel.full
	$(OBJDUMP) -dC out/kernel/kernel.full > out/kernel/kernel.asm
	$(OBJCOPY) --only-keep-debug out/kernel/kernel.full out/kernel/kernel.sym
	$(OBJCOPY) --strip-debug out/kernel/kernel.full out/kernel/kernel

ifeq ($(USB_BOOT), 1)
out/boot/bootloader.bl: out/boot/usb_bootloader.o out/boot/usb_load_kernel_and_enter.o
else
out/boot/bootloader.bl: out/boot/bootloader.o out/boot/load_kernel_and_enter.o
endif
	mkdir -p $(dir $@)
	$(LD) $^ -Ttext 0x7c00 -e entry -o out/boot/bootloader.elf
	$(OBJDUMP) -dC out/boot/bootloader.elf > out/boot/bootloader.asm
	$(OBJCOPY) -O binary out/boot/bootloader.elf out/boot/bootloader
	cp out/boot/bootloader out/boot/bootloader.bl
	python3 boot/adjust.py out/boot/bootloader.bl

img out/kernel.img: out/kernel/kernel out/boot/bootloader.bl
	kernel/check_kernel_size
	cat out/boot/bootloader.bl out/kernel/kernel > out/kernel.img
	truncate -s `printf "%d\n" 0xE0200` out/kernel.img

# It's interesting to change QEMU source code by adding debugging code. E.g.
# adding debugging code in hw/net/e1000.c in QEMU helps debugging the NIC driver.
# Set QEMU_PREFIX to the directory of the installed QEMU if needed
QEMU=$(QEMU_PREFIX)qemu-system-x86_64

ifeq ($(USB_BOOT), 1)
XHCI_OPTIONS := -device qemu-xhci,id=id_xhci -drive if=none,id=usbstick_2,format=raw,file=out/usb.img -device usb-storage,bus=id_xhci.0,drive=usbstick_2
else
UHCI_OPTIONS := -device piix4-usb-uhci,id=id_uhci -drive if=none,id=usbstick,format=raw,file=kernel/kernel_main.cpp -device usb-storage,bus=id_uhci.0,drive=usbstick

XHCI_OPTIONS := -device qemu-xhci,id=id_xhci -drive if=none,id=usbstick_2,format=raw,file=kernel/kernel_main.cpp -device usb-storage,bus=id_xhci.0,drive=usbstick_2
endif

USB_OPTIONS := $(UHCI_OPTIONS) $(XHCI_OPTIONS)

ifeq ($(USB_BOOT), 1)
# usb boot don't need extra image options since everything is loaded from  usb
img_options :=
else
img_options := -hda out/kernel.img -hdb fs.img
endif

# We put the filesystem in hdb rather than put it together with the kernel in hda
# to make it easy to recreate kernel image while keeping the fs image.
run: build
	# the options to setup monitor by telnet is copied from https://stackoverflow.com/questions/49716931/how-to-run-qemu-with-nographic-and-monitor-but-still-be-able-to-send-ctrlc-to
	# Using -nographic option does not show content written to the vga memory.
	# Use -curses works. Check https://stackoverflow.com/questions/6710555/how-to-use-qemu-to-run-a-non-gui-os-on-the-terminal for details.
	# simulate 100M memory
	#
	# '-serial stdio' is added so the final content on the screen in the guest OS is available in the host terminal after terminating qemu.
	#   One flaw: after adding this option, the first keyboard input is somehow lost.
	$(QEMU) -display curses -monitor telnet::2000,server,nowait $(img_options) -m 100 -no-reboot -no-shutdown $(USB_OPTIONS) -device e1000,netdev=id_net -netdev user,id=id_net,hostfwd=tcp::8080-:80 -object filter-dump,id=id_filter_dump,netdev=id_net,file=/tmp/dump.dat $(QEMU_EXTRA)

ifeq ($(USB_BOOT), 1)
build: out/usb.img

out/usb.img: out/kernel.img fs.img
	cp out/kernel.img out/usb.img
	truncate -s `printf "%d\n" 0x100000` out/usb.img
	# the file system starts at 1MB offset
	cat fs.img >> out/usb.img
else
build: out/kernel.img fs.img
endif

# start a connection to qemu monitor
mon:
	nc localhost 2000

# run qemu in graphic mode
run-graph: out/kernel.img fs.img
	qemu-system-x86_64 -hda out/kernel.img -hdb fs.img

# will not recreate if fs.img already exists. This makes sure we don't drop mutations in the filesystem.
# Not puting fs.img under out/ on purpose so 'make clean' does not drop it
fsimg fs.img:
	mkdir -p out/fs_template
	echo "Hello, simfs!" > out/fs_template/message
	$(MAKE) one # don't put one as a prerequisite on purpose so fs.img does not get overriden whenever one needs to be rebuilt
	$(MAKE) out/user/shell
	$(MAKE) out/user/test_fork
	$(MAKE) out/user/test_readfile
	$(MAKE) out/user/test_writefile
	$(MAKE) out/user/ls
	$(MAKE) out/user/echo
	$(MAKE) out/user/mkdir
	$(MAKE) out/user/cat
	cp out/user/one out/fs_template
	cp out/user/shell out/fs_template
	cp out/user/test_fork out/fs_template
	cp out/user/test_readfile out/fs_template
	cp out/user/test_writefile out/fs_template
	cp out/user/ls out/fs_template
	cp out/user/echo out/fs_template
	cp out/user/mkdir out/fs_template
	cp out/user/cat out/fs_template
	cp out/kernel/kernel.sym out/fs_template
	python3 mkfs.py out/fs_template fs.img $(MKFS_EXTRA) # python points to python2 in make's shell instance but point to python3.6 outside of make. I have to explicitly specify python3.6 for now since mkfs.py requires python3. TODO: figure out the root cause

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

# user application
one:
	mkdir -p out/user
	$(CC) -c user/one.s -o out/user/one.o
	$(LD) out/user/one.o -o out/user/one -e entry -Tulib/user.ld

out/user/%: out/user/%.o $(ULIB_OBJ) $(CLIB_OBJ)
	$(LD) $^ -o $@ -e entry -Tulib/user.ld
	$(OBJDUMP) -dC $@ > $@.asm
