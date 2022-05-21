IMHO, inline assembly is a hacky technique since it's quite hard to fully understand and control what's happening. I use it in the project for now just to understand this technique deeper. I may remove them in future though.

Basic inline assembly can be used to define functions in C/C++ global scope. This is pretty cool.

Extended inline assembly is quite counter-intuitive and hard to make it right.

# volatile qualifier
Add the volatile qualifier to avoid an inline assembly block being moved or removed by the optimizer. Usage looks like
```
asm volatile(...);
```

# Misc
```
Q: Will constraint "r" ever conflict with a register used(read/write) in the 'assembly template'?
```
```
A: It's possible. Use clobber list to avoid issue.
```

# References
- [Inline assembler - wikipedia](https://en.wikipedia.org/wiki/Inline_assembler)
- [Inline Assembly - osdev](https://wiki.osdev.org/Inline_Assembly)
- [Inline assembly - cppreference](https://en.cppreference.com/w/c/language/asm)
- [GCC Inline Assembly HOWTO](http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html)
- [clang compatibility](https://clang.llvm.org/compatibility.html#inline-asm): clang is highly compatible with the GCC inline assembly extensions.
- [GCC Official Doc - How to use inline assembly language in C code](https://gcc.gnu.org/onlinedocs/gcc/Using-Assembly-Language-with-C.html#Using-Assembly-Language-with-C)
