include config.mk

UCC_CC = ../src/ucc/ucc

LIBC   = libc.a
CRT    = crt.a

OBJ_LIBC_C =                                              \
           stdio.o stdlib.o string.o unistd.o signal.o    \
           assert.o ctype.o dirent.o stdarg.o             \
           sys/fcntl.o sys/wait.o sys/mman.o              \
           sys/utsname.o sys/select.o sys/time.o          \
           arpa/inet.o

OBJ_LIBC_S = syscall.o alloca.o

OBJ_CRT  = crt.o begin.o
OBJ_LIBC = ${OBJ_LIBC_C} ${OBJ_LIBC_S}

CPPFLAGS = -nostdinc
CFLAGS = -target ${TARGET} -g -Wmissing-forwards

LIBDEPS = ${UCC_CC} ../src/cc1/cc1 ../src/cpp2/cpp

CC = ${UCC_CC}

ARCH_DIR = arch/${ARCH}

.PHONY: all clean

all: ${LIBC} ${CRT}

${CRT}: ${OBJ_CRT}
	${AR} rc ${CRT} ${OBJ_CRT}
${LIBC}: ${OBJ_LIBC}
	${AR} rc ${LIBC} ${OBJ_LIBC}

%.o: ${ARCH_DIR}/%.S
	${UCC_CC} -target ${TARGET} ${CPPFLAGS} -c -o $@ $<

%.o: %.c
	${UCC_CC} ${CPPFLAGS} ${CFLAGS} -c -o $@ $<

%.s: %.c
	${UCC_CC} ${CPPFLAGS} ${CFLAGS} -S -o $@ $<

string_strerrs: string_strerrs.c
	${CC} -o $@ $<

string_strerrs.h: string_strerrs
	./string_strerrs > $@.tmp
	mv -f $@.tmp $@

syscalls_darwin.h: syscalls_darwin_unix.h
	@# 0x2000000 in decimal = 33554432
	awk '/[0-9]+ *$$/ { $$NF += 33554432 } 1' < $< > $@.tmp
	mv -f $@.tmp $@

clean:
	rm -f ${LIBC} ${CRT} ${OBJ_CRT} ${OBJ_LIBC} \
		string_strerrs string_strerrs.h syscalls_darwin.h \
		.*.d

# dependencies
${OBJ_LIBC}: ${LIBDEPS}
${OBJ_CRT}: ${LIBDEPS}

${OBJ_LIBC_C}: syscalls_darwin.h string_strerrs.h

.%.d: %.c
	${UCC_CC} -MM -MG ${CPPFLAGS} $< > $@.tmp
	mv -f $@.tmp $@

-include ${OBJ_LIBC_C:%.o=.%.d}
