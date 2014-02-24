UCC = ./ucc
GCC = gcc-4.9
LD = ${GCC}

OUTS = struct_call.ucc_i.gcc_c.out struct_call.gcc_i.ucc_c.out \
	struct_call.ucc_i.ucc_c.out struct_call.gcc_i.gcc_c.out

all: ${OUTS}

clean:
	rm -f ${OUTS} struct_call*.o

struct_call.ucc_i.gcc_c.out: struct_call.ucc_i.o struct_call.gcc_c.o
	${LD} -o $@ $^

struct_call.gcc_i.ucc_c.out: struct_call.gcc_i.o struct_call.ucc_c.o
	${LD} -o $@ $^

struct_call.ucc_i.ucc_c.out: struct_call.ucc_i.o struct_call.ucc_c.o
	${LD} -o $@ $^

struct_call.gcc_i.gcc_c.out: struct_call.gcc_i.o struct_call.gcc_c.o
	${LD} -o $@ $^

struct_call.gcc_c.o: struct_call.c
	${GCC} -c -o $@ $<
struct_call.gcc_i.o: struct_call.c
	${GCC} -c -DI -o $@ $<
struct_call.ucc_c.o: struct_call.c
	${UCC} -c -o $@ $<
struct_call.ucc_i.o: struct_call.c
	${UCC} -c -DI -o $@ $<
