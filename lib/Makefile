CC  = ../src/cc1
OBJ = crt.o print.o

libs: ${OBJ}

crt.o: crt.s
	nasm -f elf64 -o $@ $<
print.o: print.s
	nasm -f elf64 -o $@ $<

print.s: print.c
	../src/cc1 $< > $@

clean:
	rm -f *.o print.s

.PHONY: libs clean
