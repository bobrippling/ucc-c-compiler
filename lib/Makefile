CC  = ../src/cc1
OBJ = start.o crt.o print.o

include ../config.mk

libs: ${OBJ}

crt.o: crt.s
	${NASM} -f ${ARCH} -o $@ $<
start.o: start.s
	${NASM} -f ${ARCH} -o $@ $<
print.o: print.s
	${NASM} -f ${ARCH} -o $@ $<

start.s: start.c
	../src/cc1 $< > $@
print.s: print.c
	../src/cc1 $< > $@

clean:
	rm -f *.o print.s start.s

.PHONY: libs clean
