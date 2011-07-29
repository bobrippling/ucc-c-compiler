OBJ = 1.o printd.o crt.o

test: ${OBJ}
	ld -o $@ ${OBJ}

1.o: 1.s
	nasm -f elf64 -o $@ $<
printd.o: printd.s
	nasm -f elf64 -o $@ $<
crt.o: crt.s
	nasm -f elf64 -o $@ $<

1.s: ../INPUT.c ../cc1
	../cc1 $< > $@
printd.s: printd.c ../cc1
	../cc1 $< > $@

clean:
	rm -f *.o

.PHONY: clean
