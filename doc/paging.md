# Motivation
We have to implement paging to support multi-processing. Without paging, kernel and all processes will need to live in the same address space. This has big drawbacks:

- a process can not be loaded to a picked address at linking time. Either load time relocation or position-independent-code need to be supported
- there will be no isolation between different processes or between a process and the kernel.

# Split Between User Space and Kernel Space
How should we cut the whole linear address space for user space and kernel space? Linux kernel reserve space below 0xC0000000 for user space and the rest high address space for the kernel. That should be good to maintain backward compatibility to run old applications.

However, I think it's more natual to reserve low address space (e.g. those below 0x40000000) for kernel and high address space for user space. This way, we can use an identity paging for kernel address space: e.g. kernel linear address x will be mapped to physical address x. The drawback of this solution is about portability: we may not be able to run binaries for linux directly on SOS. I still want to implement it as an experiment. It should be easy to add the same implementation as Linux later and make it configuratble to pick which solution to use.

# Reference
Check 'Chapter 4: Paging' of the intel developer manual volume 3 for details.
