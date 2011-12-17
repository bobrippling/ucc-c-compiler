UCC_CC = ../src/cc
OBJ    = syscall.o crt.o unistd.o stdio.o string.o \
				 stdlib.o sys/fcntl.o sys/wait.o

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
sys/fcntl.o: sys/fcntl.c
	${BUILD_CMD}
sys/wait.o: sys/wait.c
	${BUILD_CMD}

syscall.s: syscall_in.s syscall_err.s
	../src/cpp/cpp -o $@ syscall_in.s

clean:
	rm -f *.o sys/*.o

.PHONY: libs clean

stdio.o: stdio.c unistd.h stdio.h string.h
stdlib.o: stdlib.c stdlib.h syscalls.h os.h unistd.h
string.o: string.c string.h string_strerrs.h
unistd.o: unistd.c unistd.h syscalls.h os.h
fcntl.o: sys/fcntl.c sys/fcntl.h syscalls.h os.h
wait.o: sys/wait.c sys/wait.h syscalls.h os.h
