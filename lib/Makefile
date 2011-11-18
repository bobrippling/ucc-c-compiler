UCC_CC = ../src/cc
OBJ    = syscall.o crt.o unistd.o stdio.o string.o stdlib.o fcntl.o

LIBDEPS = ${UCC_CC} ../src/cc1/cc1 ../src/cpp/cpp

BUILD_CMD = ${UCC_CC} -c -o $@ $<

include ../config.mk

libs: ${OBJ}

crt.o: crt.s
	${BUILD_CMD}
syscall.o: syscall.s
	${BUILD_CMD}

stdio.o: stdio.c
	${BUILD_CMD}
stdlib.o: stdlib.c
	${BUILD_CMD}
string.o: string.c
	${BUILD_CMD}
unistd.o: unistd.c
	${BUILD_CMD}
fcntl.o: fcntl.c
	${BUILD_CMD}

syscall.s: syscall_in.s syscall_err.s
	../src/cpp/cpp -o $@ $<

clean:
	rm -f *.o

.PHONY: libs clean

stdio.o: stdio.c unistd.h ${LIBDEPS}
stdlib.o: stdlib.c stdlib.h syscalls.h os.h ${LIBDEPS}
string.o: string.c string.h ${LIBDEPS}
unistd.o: unistd.c unistd.h syscalls.h os.h ${LIBDEPS}
