CC  = ../src/cc
OBJ = syscall.o crt.o unistd.o stdio.o string.o stdlib.o

BUILD_NASM = ${UCC_NASM} -f ${UCC_ARCH} -o $@ $<

include ../config.mk

libs: ${OBJ} ${CC}

%.o: %.s
	${BUILD_NASM}

%.s: %.c
	${CC} $< > $@

clean:
	rm -f *.o unistd.s stdio.s

.PHONY: libs clean

stdio.o: stdio.c unistd.h ${CC}
stdlib.o: stdlib.c stdlib.h syscalls.h os.h ${CC}
string.o: string.c string.h ${CC}
unistd.o: unistd.c unistd.h syscalls.h os.h ${CC}
