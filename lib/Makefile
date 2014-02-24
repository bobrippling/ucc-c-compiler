UCC_CC = ../src/ucc/ucc
LIBC   = libc.a
CRT    = crt.a
OBJ_LIBC = syscall.o begin.o                              \
           alloca.o                                       \
           stdio.o stdlib.o string.o unistd.o signal.o    \
           assert.o ctype.o dirent.o                      \
           sys/fcntl.o sys/wait.o sys/mman.o sys/socket.o \
           sys/utsname.o sys/select.o sys/time.o          \
           arpa/inet.o

OBJ_CRT  = crt.o

LIBDEPS = ${UCC_CC} ../src/cc1/cc1 ../src/cpp2/cpp

CPPFLAGS = -nostdinc
CFLAGS = -g

BUILD_CMD = ${UCC_CC} ${CPPFLAGS} ${CFLAGS} -c -o $@ $<

include ../config.mk

ARCH_DIR = arch/${UCC_SUBARCH}

all: ${LIBC} ${CRT}

${CRT}: ${OBJ_CRT}
	${AR} rc ${CRT} ${OBJ_CRT}
${LIBC}: ${OBJ_LIBC}
	${AR} rc ${LIBC} ${OBJ_LIBC}

${OBJ_LIBC}: ${LIBDEPS}
${OBJ_CRT}: ${LIBDEPS}

crt.o: ${ARCH_DIR}/crt.S
	${BUILD_CMD}
syscall.o: ${ARCH_DIR}/syscall.S
	${BUILD_CMD}
begin.o: begin.c
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
dirent.o: dirent.c
	${BUILD_CMD}

alloca.o: ${ARCH_DIR}/alloca.S
	${BUILD_CMD}

#ucc.o: ucc.c
#	${UCC_CC} -fenable-asm -c -o $@ $<

sys/fcntl.o: sys/fcntl.c
	${BUILD_CMD}
sys/wait.o: sys/wait.c
	${BUILD_CMD}
sys/mman.o: sys/mman.c
	${BUILD_CMD}
sys/socket.o: sys/socket.c
	${BUILD_CMD}
sys/utsname.o: sys/utsname.c
	${BUILD_CMD}
sys/select.o: sys/select.c
	${BUILD_CMD}
sys/time.o: sys/time.c
	${BUILD_CMD}

arpa/inet.o: arpa/inet.c
	${BUILD_CMD}

clean:
	rm -f *.o sys/*.o ${CRT} ${LIBC}

.PHONY: all clean



assert.o: assert.c stdio.h stdarg.h sys/types.h macros.h stdlib.h \
 assert.h
ctype.o: ctype.c ctype.h stdlib.h sys/types.h macros.h ucc_attr.h
dirent.o: dirent.c dirent.h sys/types.h sys/fcntl.h unistd.h macros.h \
 stdlib.h ucc_attr.h
signal.o: signal.c signal.h unistd.h macros.h sys/types.h sys/signal.h \
 syscalls.h syscalls_64.h
stdio.o: stdio.c unistd.h macros.h sys/types.h stdio.h stdarg.h string.h \
 errno.h stdlib.h sys/fcntl.h assert.h ctype.h ucc_attr.h
stdlib.o: stdlib.c stdlib.h sys/types.h macros.h unistd.h syscalls.h \
 syscalls_64.h signal.h sys/signal.h string.h assert.h errno.h ucc_attr.h \
 sys/mman.h sys/types.h
string.o: string.c string.h macros.h sys/types.h limits.h \
 string_strerrs.h
unistd.o: unistd.c unistd.h macros.h sys/types.h syscalls.h syscalls_64.h
ucc.o: ucc.c ucc.h

arpa/inet.o: arpa/inet.c arpa/inet.h

sys/fcntl.o: sys/fcntl.c sys/fcntl.h sys/../stdarg.h sys/../sys/types.h \
 sys/../syscalls.h sys/../syscalls_64.h
sys/mman.o: sys/mman.c sys/types.h sys/mman.h sys/../syscalls.h \
 sys/../syscalls_64.h
sys/socket.o: sys/socket.c sys/socket.h sys/types.h sys/../syscalls.h \
 sys/../syscalls_64.h
sys/wait.o: sys/wait.c sys/wait.h sys/../unistd.h sys/../macros.h \
 sys/../sys/types.h sys/../syscalls.h sys/../syscalls_64.h
