include ../src/config.mk
# include src_config (for $ARCH), but overwrite CFLAGS, etc...

UCC_CC = ../src/ucc/ucc
LIBC   = libc.a
CRT    = crt.a
OBJ_LIBC = syscall.o begin.o                              \
           alloca.o                                       \
           stdio.o stdlib.o string.o unistd.o signal.o    \
           assert.o ctype.o dirent.o stdarg.o             \
           sys/fcntl.o sys/wait.o sys/mman.o sys/socket.o \
           sys/utsname.o sys/select.o sys/time.o          \
           arpa/inet.o

OBJ_CRT  = crt.o

LIBDEPS = ${UCC_CC} ../src/cc1/cc1 ../src/cpp2/cpp

CPPFLAGS = -nostdinc
CFLAGS = -g -Wmissing-forwards

CC = ${UCC_CC}

ARCH_DIR = arch/${ARCH}

all: ${LIBC} ${CRT}

${CRT}: ${OBJ_CRT}
	${AR} rc ${CRT} ${OBJ_CRT}
${LIBC}: ${OBJ_LIBC}
	${AR} rc ${LIBC} ${OBJ_LIBC}

${OBJ_LIBC}: ${LIBDEPS}
${OBJ_CRT}: ${LIBDEPS}

crt.o: ${ARCH_DIR}/crt.S
	${CC} -c -o $@ $< ${ASFLAGS}
syscall.o: ${ARCH_DIR}/syscall.S
	${CC} -c -o $@ $< ${ASFLAGS}
alloca.o: ${ARCH_DIR}/alloca.S
	${CC} -c -o $@ $< ${ASFLAGS}

clean:
	rm -f *.o sys/*.o ${CRT} ${LIBC}

.PHONY: all clean

assert.o: assert.c stdio.h stdarg.h sys/types.h macros.h stdlib.h \
 ucc_attr.h assert.h
begin.o: begin.c unistd.h macros.h sys/types.h sys/time.h ucc_attr.h \
 stdlib.h
ctype.o: ctype.c ctype.h stdlib.h sys/types.h macros.h ucc_attr.h
dirent.o: dirent.c dirent.h sys/types.h sys/fcntl.h unistd.h macros.h \
 sys/time.h ucc_attr.h stdlib.h errno.h
signal.o: signal.c signal.h unistd.h macros.h sys/types.h sys/time.h \
 ucc_attr.h sys/signal.h syscalls.h syscalls_64.h
stdarg.o: stdarg.c stdarg.h sys/types.h
stdio.o: stdio.c unistd.h macros.h sys/types.h sys/time.h ucc_attr.h \
 stdio.h stdarg.h string.h errno.h stdlib.h sys/fcntl.h assert.h ctype.h \
 limits.h
stdlib.o: stdlib.c stdlib.h sys/types.h macros.h ucc_attr.h unistd.h \
 sys/time.h syscalls.h syscalls_64.h signal.h sys/signal.h string.h \
 assert.h errno.h sys/mman.h sys/types.h
string.o: string.c string.h macros.h sys/types.h limits.h stdlib.h \
 ucc_attr.h string_strerrs.h
ucc.o: ucc.c stdio.h stdarg.h sys/types.h macros.h stdlib.h ucc_attr.h \
 ucc.h
unistd.o: unistd.c unistd.h macros.h sys/types.h sys/time.h ucc_attr.h \
 syscalls.h syscalls_64.h sys/select.h sys/types.h sys/time.h

arpa/inet.o: arpa/inet.c arpa/inet.h

sys/fcntl.o: sys/fcntl.c sys/fcntl.h sys/../stdarg.h sys/../sys/types.h \
 sys/../syscalls.h sys/../syscalls_64.h
sys/mman.o: sys/mman.c sys/types.h sys/mman.h sys/../syscalls.h \
 sys/../syscalls_64.h
sys/socket.o: sys/socket.c sys/socket.h sys/types.h sys/../syscalls.h \
 sys/../syscalls_64.h
sys/wait.o: sys/wait.c sys/wait.h sys/../unistd.h sys/../macros.h \
 sys/../sys/types.h sys/../syscalls.h sys/../syscalls_64.h
