After building your kernel image, the next step is to flash it into a usb drive and boot from it. This doc contains information about how to do that.

# balenaEtcher

I use this a lot in my old x86-64 macbook. This is a great tool with clean and simple GUI. But unfortunately there is no arm support yet. So one can not use it on an M1 macbook yet.

# dd

dd is a unix command to copy data around. It supports copying data into a device file besides regular file. So we can use it to copy the kernel image to the usb drive.

To use dd to flash the usb drive, we need know the device file for the usb drive in the first place. On macos, run

```
diskutil list
```

to find out the device file.

And then run dd like

```
sudo dd if=image-file of=usb-device-file
```

to flash the usb drive.
