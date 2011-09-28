CC  = ../src/cc
OBJ = syscall.o crt.o unistd.o stdio.o string.o stdlib.o

BUILD_NASM = ${NASM} -f ${ARCH} -o $@ $<

include ../config.mk

libs: ${OBJ} ${CC}

%.o: %.s
	${BUILD_NASM}

%.s: %.c
	${CC} $< > $@

clean:
	rm -f *.o unistd.s stdio.s

.PHONY: libs clean
