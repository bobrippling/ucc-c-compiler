CC  = ../src/cc1
OBJ = start.o crt.o print.o

libs: ${OBJ}

crt.o: crt.s
	nasm -f elf64 -o $@ $<
start.o: start.s
	nasm -f elf64 -o $@ $<
print.o: print.s
	nasm -f elf64 -o $@ $<

start.s: start.c
	../src/cc1 $< > $@
print.s: print.c
	../src/cc1 $< > $@

clean:
	rm -f *.o print.s

.PHONY: libs clean
