If you develop the OS on systems such as MacOS that does not support ELF, you need a cross-compiling tool chain. This tutorial focuses on MacOS.

# Using brew

`brew install i686-elf-gcc` will install the elf-gcc for x86. There are packages for i686-elf-gdb etc. and even packages for `x86_64-elf-gcc`.

This is the easiest way to install the cross-compiler.

# Install the cross compiler from source

This is the more complex way and it's not even work for now. Write this down for recording purpose only.

## preparation step

```

# export TARGET=i386-elf # does not work. i386-elf-gcc still generate Mach-O file.
export TARGET=i686-elf # also not work. i686-elf-gcc still generate Mach-O file.

# install libs needed by gcc
brew install gmp mpfr libmpc

# have to use gcc (not clang) to compile the cross-compiling gcc
export CC=`which gcc-11` # replace this to the gcc you installed
export LD=`which gcc-11`
```

## binutils

```
wget https://ftp.gnu.org/gnu/binutils/binutils-2.37.tar.xz # this is the latest version when I wrote this
tar -xvf binutils-2.37.tar.xz
cd binutils-2.37
mkdir -p build/prefix
cd build
time ../configure --target=$TARGET --enable-interwork --enable-multilib --disable-nls --disable-werror --prefix=`pwd`/prefix
time make -j
time make install
```

## GCC 

```
wget http://mirror.0xem.ma/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.xz # this is the latest version when I wrote this
tar -xvf gcc-11.2.0.tar.xz
cd gcc-11.2.0
mkdir -p build/prefix
cd build
# may need to add gmp/mpfr/mpc path on the cmdline explicitly if complained. Check the error message for details.
time ../configure --target=$TARGET --prefix=`pwd`/prefix --disable-nls --disable-libssp --enable-languages=c --without-headers
time make all-gcc -j
time make install-gcc
```

## Setup PATH
You may want to add the installed i386-elf tools to PATH for convenience.

## Reference
- [os-tutorial github](https://github.com/cfenollosa/os-tutorial/tree/master/11-kernel-crosscompiler)
