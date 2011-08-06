CC  = ../src/cc1
OBJ = crt.o printd.o

libs: ${OBJ}

crt.o: crt.s
	nasm -f elf64 -o $@ $<
printd.o: printd.s
	nasm -f elf64 -o $@ $<

printd.s: printd.c
	../src/cc1 $< > $@

clean:
	rm -f *.o printd.s

.PHONY: libs clean
