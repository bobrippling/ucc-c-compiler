SRC = pack.c pack2.c
OBJ_UCC = pack_ucc.o pack2_ucc.o
OBJ_GCC = pack_gcc.o pack2_gcc.o

UCC = ../../ucc

all: gcc ucc gcc_ucc ucc_gcc

gcc: ${OBJ_GCC}
	cc -o $@ ${OBJ_GCC}

ucc: ${OBJ_UCC}
	${UCC} -o $@ ${OBJ_UCC}

gcc_ucc: pack_gcc.o pack2_ucc.o
	cc -o $@ $^

ucc_gcc: pack_ucc.o pack2_gcc.o
	cc -o $@ $^


pack_gcc.o: pack.c
	cc -c -o $@ $<
pack2_gcc.o: pack2.c
	cc -c -o $@ $<

pack_ucc.o: pack.c
	${UCC} -c -o $@ $<
pack2_ucc.o: pack2.c
	${UCC} -c -o $@ $<

clean:
	rm -f gcc ucc gcc_ucc ucc_gcc *.o

pack.o: pack.c pack.h
pack2.o: pack2.c pack.h
