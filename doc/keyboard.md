# Keyboard Driver
Keyboard controller uses IRQ 1. When an interrupt happens, the driver can read the information about the key in 0x60 port.

# Scancode
Keyboard uses scancode to encode keys being pressed or releases. Scancode encodes the position of a key in the keyboard. There are 3 sets (versions) of scancode. Some hardward that natively use a differet set of scancode will automatically convert that to set 1.

When a key is pressed, a scan code x will be generated (sometimes a sequence of scan codes will be generated). x will have the 7th bit cleared. When the key is releases, a scan code `x | 0x80` will be generated.

# A20 Gate
To be backward-compatible with old real mode software that assumes memory address beyond 1M being wrap around to the low address, a A20 gate is used. When A20 gate is closed, A20 address line will always be 0; only when the gate is open, software can access addresses with A20 to be 1. A20 gate is implemented in the keyboard controller.

# Reference
Note, it seems hard to find the official doc/datasheet for the keyboard controller that explains how to write a keyboard driver. Here are the materials I found on the internet.

- [interrupts - osdev wiki](https://wiki.osdev.org/Interrupts): it explains for keyboard interrupt, the handler needs talk to the `keyboard` via port 0x60 to know the key.
- [os-tutorial](https://github.com/cfenollosa/os-tutorial/blob/master/20-interrupts-timer/drivers/keyboard.c#L8) repo has code that reads scancode from 0x60 port in the keyboard interrupt handler.
- [PS/2 keyboard and mouse](https://en.wikipedia.org/wiki/PS/2_port)
- [linux kernel keyboard driver](https://github.com/torvalds/linux/blob/master/drivers/input/keyboard/atkbd.c)
- [map scancode to ascii](http://www.philipstorr.id.au/pcbook/book3/scancode.htm)
- [PS/2 Controller - osdev wiki](https://wiki.osdev.org/%228042%22_PS/2_Controller)
- [Keyboard Controller - wikipedia](https://en.wikipedia.org/wiki/Keyboard_controller_(computing))
- [Scancode - wikipedia](https://en.wikipedia.org/wiki/Scancode)
- [A20 line](https://en.wikipedia.org/wiki/A20_line)
