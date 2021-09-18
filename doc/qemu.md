# Install
`$ brew install qemu`

# Find the Location of the Installed qemu
`$ brew info qemu`

# Simulate x86_64
The installed qemu comes with multiple executables one for each supported processors. For x86_64, the executable name is `qemu-system-x86_64`.

Use `-hda` option to specify the boot image. Here is an example command:
```
  qemu-system-x86_64 -hda out/boot/hello.bl
```
This is also the command used in `boot/Makefile` to launch qemu.

# qemu monitor
qemu monitor is very helpful for debugging. We can use qemu monitor to inspect CPU registers and memory.

Type 'ctrl-option-2' to enter qeum monitor. Type 'ctrl-option-1' to go back to the console.
Type 'ctrl-shift-up/down' to scroll up or down in qemu monitor.

Here are some useful qemu monitor commands:

- 'x addr': check the contennt of the memory address
- 'print $eax': print eax register
- 'print $cs': print cs register
- 'print $eflags': print eflags register
- 'print /d sth': print in decimal foramt
- 'print /x sth': print in hexadecimal foramt
