UCC_CC = ../src/cc
OBJ    = crt.o syscall.o \
         stdio.o stdlib.o string.o unistd.o signal.o \
				 assert.o ctype.o \
				 ucc.o \
         sys/fcntl.o sys/wait.o sys/mman.o sys/socket.o

LIBDEPS = ${UCC_CC} ../src/cc1/cc1 ../src/cpp2/cpp

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
ctype.o: ctype.c
	${BUILD_CMD}
ucc.o: ucc.c
	${UCC_CC} -fenable-asm -Wno-optimisation -c -o $@ $<

sys/fcntl.o: sys/fcntl.c
	${BUILD_CMD}
sys/wait.o: sys/wait.c
	${BUILD_CMD}
sys/mman.o: sys/mman.c
	${BUILD_CMD}
sys/socket.o: sys/socket.c
	${BUILD_CMD}

syscall.s: syscall_in.s syscall_err.s
	${UCC_CC} -x c -E -o $@ syscall_in.s

clean:
	rm -f *.o sys/*.o

.PHONY: libs clean


assert.o: assert.c stdio.h stdarg.h sys/types.h macros.h stdlib.h \
 assert.h
ctype.o: ctype.c ctype.h stdlib.h sys/types.h macros.h
signal.o: signal.c signal.h unistd.h macros.h sys/signal.h syscalls.h \
 syscalls_64.h
stdio.o: stdio.c unistd.h macros.h stdio.h stdarg.h sys/types.h string.h \
 errno.h stdlib.h sys/fcntl.h assert.h ctype.h
stdlib.o: stdlib.c stdlib.h sys/types.h macros.h unistd.h syscalls.h \
 syscalls_64.h signal.h sys/signal.h string.h assert.h sys/mman.h \
 sys/types.h
string.o: string.c string.h macros.h sys/types.h limits.h \
 string_strerrs.h
ucc.o: ucc.c ucc.h
unistd.o: unistd.c unistd.h macros.h syscalls.h syscalls_64.h

sys/fcntl.o: sys/fcntl.c sys/fcntl.h sys/../stdarg.h sys/../sys/types.h \
 sys/../syscalls.h sys/../syscalls_64.h
sys/mman.o: sys/mman.c sys/types.h sys/mman.h sys/../syscalls.h \
 sys/../syscalls_64.h
sys/wait.o: sys/wait.c sys/wait.h sys/../unistd.h sys/../macros.h \
 sys/../syscalls.h sys/../syscalls_64.h
