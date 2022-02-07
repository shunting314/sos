This doc lists the references for building SOS.

# os-tutorial

This is a [github repository](https://github.com/cfenollosa/os-tutorial). It has 21K stars. It's very easy to follow.

Pros:

- load the kernel without leveraging GRUB
- the author also uses Mac to develop the OS. There is a chapter to describe how to setup cross-compiling on Mac!

Cons:

- lack some important features like page management, process, filesystem, network etc.
- some features are implemented in a too simple way. E.g., kmalloc does not have a corresponding kfree implementation.
