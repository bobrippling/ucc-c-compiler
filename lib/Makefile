UCC_CC = ../src/cc
OBJ    = syscall.o crt.o unistd.o stdio.o string.o stdlib.o fcntl.o

LIBDEPS = ${UCC_CC} ../src/cc1/cc1 ../src/cpp/cpp

BUILD_CMD = ${UCC_CC} -c -o $@ $<

include ../config.mk

libs: ${OBJ}

${OBJ}: ${LIBDEPS}

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
	../src/cpp/cpp -o $@ syscall_in.s

clean:
	rm -f *.o

.PHONY: libs clean

fcntl.o: fcntl.c fcntl.h syscalls.h os.h
stdio.o: stdio.c unistd.h stdio.h string.h
stdlib.o: stdlib.c stdlib.h syscalls.h os.h
unistd.o: unistd.c unistd.h syscalls.h os.h
