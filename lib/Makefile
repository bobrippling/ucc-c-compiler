OBJ = crt.o printd.o

libs: ${OBJ}

printd.o: printd.s
	nasm -f elf64 -o $@ $<
crt.o: crt.s
	nasm -f elf64 -o $@ $<

printd.s: printd.c
	../src/cc1 $< > $@

clean:
	rm -f *.o printd.s

.PHONY: libs clean
