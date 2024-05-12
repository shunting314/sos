# Boot related problem trouble shooting

I once got the following error when booting debian:
```
Loading Linux 6.2.0+ ...
Loading initial ramdisk ...
error: couldn't find suitable memory target.
```

The error message "error: couldn't find suitable memory target." is from GRUB since I can grep it from GRUB source code.

Google the error message, I found this [answer](https://superuser.com/questions/1294064/grub-ubuntu-couldnt-find-suitable-memory-target-after-kernel-upgrade). Some people found the root cause is that the initrd image is too large. I tried the solution to strip symbols in loadable kernel modules by adding `INSTALL_MOD_STRIP=1` when running `make modules_install` so the command looks like `sudo make INSTALL_MOD_STRIP=1 modules_install`. That solves the problem.

Before stripping, the initrd image is 400+MB. After stripping it's about 60+MB (the same as one of the original initrd image the system has).

I encountered the issue all of a sudden. The error starts popping up when I boot from the rebuilt kernel after a long sequence of succeessful tries. I think maybe the initrd image I built get larger gradually and eventually exceed some limit and cause the issue.

# MISC things that's interesting to figure out
- is there a way to inspect the content of initrd on the already booted system?
  E.g. mount it somewhere manually. Note that when rebooting the mounted initrd image should have already been unmounted.

  Also we may want to inspect the content of an initrd image other than the one used to boot the current running kernel.

- What's the size limit of initrd image size according to GRUB?

# Reference
- About ram disk
  - [what is ram disk - kingston.com](https://www.kingston.com/en/blog/pc-performance/what-is-ram-disk)
  - [tmpfs - wikipedia](https://en.wikipedia.org/wiki/Tmpfs)
    - /tmp may or may not be a tmpfs
  - [RAM Drive - wikipedia](https://en.wikipedia.org/wiki/RAM_drive) +++++
- Linux initrd
  - [initrd - wikipedia](https://en.wikipedia.org/wiki/Initial_ramdisk)
  - [initrd - docs.kernel.org](https://docs.kernel.org/admin-guide/initrd.html)
- GRUB
  - [GRUB - wikipedia](https://en.wikipedia.org/wiki/GNU_GRUB)
    - GRUB stands for GNU GRand Unified Bootloader.
    - GRUB will load kernel and initrd. Then GRUB pass the location of the loaded
      initrd to kernel when it transfers the control to kernel.
  - [GNU GRUB Home Page](https://www.gnu.org/software/grub/index.html)
- [Linux boot process - wikipedia](https://en.wikipedia.org/wiki/Booting_process_of_Linux)
