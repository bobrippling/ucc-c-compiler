CC  = ../src/cc1
OBJ = start.o crt.o print.o

BUILD_NASM = ${NASM} -f ${ARCH} -o $@ $<

include ../config.mk

libs: ${OBJ} ${CC}

crt.o: crt.s
	${BUILD_NASM}
start.o: start.s
	${BUILD_NASM}
print.o: print.s
	${BUILD_NASM}

print.s: print.c
	${CC} print.c > $@

clean:
	rm -f *.o print.s

.PHONY: libs clean
