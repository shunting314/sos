# Read Current Vector Offset?
Vector offset can be set during initialization. According to '8259 wiki on osdev', BIOS set the vector offset to 0x08 for the master 8259A and 0x70 for the slave 8259A.  The 'Intel 8259A Spec' does not mention any ways to read those vector offsets after setting.

# Reference

- [Intel 8259A Spec](https://pdos.csail.mit.edu/6.828/2010/readings/hardware/8259A.pdf)
- [IRQ doc](https://en.wikipedia.org/wiki/Interrupt_request_(PC_architecture))
- [Programmable Interrupt Controller](https://en.wikipedia.org/wiki/Programmable_interrupt_controller)
- [Intel 8259](https://en.wikipedia.org/wiki/Intel_8259)
- [8259 wiki on osdev](https://wiki.osdev.org/8259_PIC)
