CC = ../ucc
CCDEPS = ${CC} ../src/cc1/cc1
IR = ../../../cg/ir

tim: tim-f.o tim-main.fixed.s ${CCDEPS}
	${CC} -g -o $@ tim-f.o tim-main.fixed.s

tim-main.fixed.s: tim-main.s
	sed '/rbp.*rbp/s/,/, %r8; mov %r8,/' < $< > $@.tmp
	mv $@.tmp $@

tim-main.s: tim-main.ir ${IR}
	${IR} -o $@ $<

tim-f.o: tim.c ${CCDEPS}
	${CC} -g -c -o $@ $< -DF

tim-main.ir: tim.c ${CCDEPS}
	${CC} -emit=ir -S -o $@ $< -DMAIN

clean:
	rm -f tim-main.ir tim-f.o tim-main.s tim-main.fixed.s tim

.PHONY: clean
