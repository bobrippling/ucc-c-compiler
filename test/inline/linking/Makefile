CFLAGS_GCC = -std=c99
CFLAGS_UCC =

GCC = gcc
UCC = ../../../ucc

all: inline_ucc # inline_gcc

inline_gcc: inline_gcc.o inline2_gcc.o
	${GCC} -o $@ $^

inline_ucc: inline_ucc.o inline2_ucc.o
	${UCC} -o $@ $^

# handled by gcc by default
inline_gcc.o: inline.c inline.h
	${GCC} ${CFLAGS_GCC} -c -o $@ $<
inline2_gcc.o: inline2.c inline.h
	${GCC} ${CFLAGS_GCC} -c -o $@ $<

inline_ucc.o: inline.c inline.h
	${UCC} -c -o $@ $<
inline2_ucc.o: inline2.c inline.h
	${UCC} -c -o $@ $<


clean:
	rm -f *.o inline_[ug]cc
