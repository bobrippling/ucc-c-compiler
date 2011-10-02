UCC_CC = ../src/cc
OBJ    = syscall.o crt.o unistd.o stdio.o string.o stdlib.o

LIBDEPS = ${UCC_CC} ../src/cc1/cc1 ../src/cpp/cpp

BUILD_NASM = ${UCC_NASM} -f ${UCC_ARCH} -o $@ $<
BUILD_CC   = ${UCC_CC} -c -o $@ $<

include ../config.mk

libs: ${OBJ}

crt.o: crt.s
	${BUILD_NASM}
syscall.o: syscall.s
	${BUILD_NASM}

stdio.o: stdio.c
	${BUILD_CC}
stdlib.o: stdlib.c
	${BUILD_CC}
string.o: string.c
	${BUILD_CC}
unistd.o: unistd.c
	${BUILD_CC}

clean:
	rm -f *.o

.PHONY: libs clean

stdio.o: stdio.c unistd.h ${LIBDEPS}
stdlib.o: stdlib.c stdlib.h syscalls.h os.h ${LIBDEPS}
string.o: string.c string.h ${LIBDEPS}
unistd.o: unistd.c unistd.h syscalls.h os.h ${LIBDEPS}
