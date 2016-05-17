CC = ./ucc

bitfield-test: bitfield-test.o
	${CC} -o $@ $<

bitfield-test.o: bitfield-test.s
	${CC} -o $@ $< -c

bitfield-test.s: bitfield-test.ir
	../../cg/ir $< > $@.tmp
	sed -i '' -e '/^\[/s/^/#/' $@.tmp
	mv $@.tmp $@

bitfield-test.ir: bitfield-test.c
	./ucc -S -emit=ir -o $@ $<

clean:
	rm -f bitfield-test bitfield-test.ir bitfield-test.o bitfield-test.s
