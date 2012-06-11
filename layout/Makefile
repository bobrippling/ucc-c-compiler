ARCH = macho64
NASM = ~/src/bin/nasm

a: a.o b.o
	cc -o $@ a.o b.o

a.o: a.c
	cc -c $<

b.o: b.s
	${NASM} -f ${ARCH} $< -o $@
