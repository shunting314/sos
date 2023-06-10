all: build
	./a.out ../../out/kernel/kernel

hello: build
	i686-elf-gcc -g -c hello.c -o /tmp/hello.o
	./a.out /tmp/hello.o

build:
	g++ -DHOST_OS -DDEBUG main.cpp ../../clib/dwarf.cpp -std=c++11
