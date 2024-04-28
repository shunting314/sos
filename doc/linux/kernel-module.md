This document describes how to use kernel modules in linux.

# Useful command

- lsmod: list all kernel modules loaded
- insmod <module.ko>: load the moudle via module file
- modprobe <module name>: An alternative to load kernel module. note the argument is module name rather than module file. Also this requires the module being installed to the system module folder. This can be achieved by using `make modules_install` command.
- rmmod <module.ko>: unload the module via module file
- modprobe -r <module name>: An alternative to unload a kernel module. Same requirements for loading module via modprobe applies here.
- modinfo <module.ko>: list mod information.

# Reference
- [Linux Kernel Modules Lab](https://linux-kernel-labs.github.io/refs/heads/master/labs/kernel_modules.html)
  - most of the device drivers in linux are used in the form of kernel modules.
  - The example uses `pr_debug` to print messages in module init/exit function.
    The messsage may not show up in dmesg due to the debug level.
    If that's the case, we can use printk directly.
  - I followed the tutorial and it works!
- [Kernel Module - archlinux](https://wiki.archlinux.org/title/Kernel_module)
