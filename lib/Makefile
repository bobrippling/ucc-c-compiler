UCC_CC = ../src/cc
OBJ    = crt.o syscall.o \
         stdio.o stdlib.o string.o unistd.o signal.o assert.o \
         sys/fcntl.o sys/wait.o

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
signal.o: signal.c
	${BUILD_CMD}
assert.o: assert.c
	${BUILD_CMD}
sys/fcntl.o: sys/fcntl.c
	${BUILD_CMD}
sys/wait.o: sys/wait.c
	${BUILD_CMD}

syscall.s: syscall_in.s syscall_err.s
	${UCC_CC} -x c -E -o $@ syscall_in.s

clean:
	rm -f *.o sys/*.o

.PHONY: libs clean

assert.o: assert.c stdio.h stdarg.h sys/types.h stdlib.h assert.h
signal.o: signal.c signal.h unistd.h sys/signal.h syscalls.h \
 syscalls_64.h
stdio.o: stdio.c unistd.h stdio.h stdarg.h sys/types.h string.h
stdlib.o: stdlib.c stdlib.h sys/types.h unistd.h syscalls.h syscalls_64.h \
 signal.h sys/signal.h
string.o: string.c string.h string_strerrs.h
unistd.o: unistd.c unistd.h syscalls.h syscalls_64.h
fcntl.o: sys/fcntl.c sys/fcntl.h sys/../syscalls.h sys/../syscalls_64.h
wait.o: sys/wait.c sys/wait.h sys/../unistd.h sys/../syscalls.h \
 sys/../syscalls_64.h
