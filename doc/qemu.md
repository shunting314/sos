# Install
`$ brew install qemu`

# Find the Location of the Installed qemu
`$ brew info qemu`

# Simulate `x86_64`
The installed qemu comes with multiple executables one for each supported processors. For `x86_64`, the executable name is `qemu-system-x86_64`.

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
- 'print $eax' or 'p $eax': print eax register
- 'print $cs': print cs register
- 'print $eflags': print eflags register
- 'print /d sth': print in decimal foramt
- 'print /x sth': print in hexadecimal foramt
- 'x /10i addr': disassemble 10 instructions at the specified address. Super useful
- 'info tlb': check paging

# Useful Command Line Options
## Query the Manual Regarding Devices
`$QEMU -device help`
  List all supported devices. 

`$QEMU -device DEVICE_NAME,help`
  List supported options for the specified devices.

  E.g. `$QEMU -device e1000,help` lists options for e1000; `$QEMU -device usb-storage,help` lists options for the usb storage device.

# Reference
- [QEMU - networking](https://wiki.qemu.org/Documentation/Networking): This doc mentions the default IP addresses for the guest OS (10.0.2.15) and gateway (10.0.2.2). It also mentions an option (e.g. `-object filter-dump,id=f1,netdev=u1,file=dump.dat`) to dump the network packets inside QEMU to a file.
- [QEMU - USB](https://qemu-project.gitlab.io/qemu/system/devices/usb.html): QEMU document for USB.
