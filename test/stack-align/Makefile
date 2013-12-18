UCC = ../../ucc
CC = cc
OBJ = stack-align.impl.ucc.o stack-align.main.ucc.o stack-align.impl.gcc.o stack-align.main.gcc.o

EXE = stack-align.impl_ucc.main_gcc stack-align.impl_gcc.main_ucc \
      stack-align.impl_gcc.main_gcc stack-align.impl_ucc.main_ucc

.PHONY: test all run clean

all: ${EXE}

run: all
	./stack-align.impl_ucc.main_gcc
	./stack-align.impl_gcc.main_ucc
	./stack-align.impl_gcc.main_gcc
	./stack-align.impl_ucc.main_ucc

test: run

stack-align.impl_ucc.main_gcc: stack-align.impl.ucc.o stack-align.main.gcc.o
	${CC} -o $@ $^

stack-align.impl_gcc.main_ucc: stack-align.impl.gcc.o stack-align.main.ucc.o
	${CC} -o $@ $^

stack-align.impl_gcc.main_gcc: stack-align.impl.gcc.o stack-align.main.gcc.o
	${CC} -o $@ $^

stack-align.impl_ucc.main_ucc: stack-align.impl.ucc.o stack-align.main.ucc.o
	${CC} -o $@ $^

stack-align.impl.ucc.o: stack-align.c
	${UCC} -o $@ $< -c -DIMPL
stack-align.main.ucc.o: stack-align.c
	${UCC} -o $@ $< -c

stack-align.impl.gcc.o: stack-align.c
	${CC} -o $@ $< -c -DIMPL
stack-align.main.gcc.o: stack-align.c
	${CC} -o $@ $< -c

clean:
	rm -f                      \
		stack-align.impl.gcc.o           \
		stack-align.impl.ucc.o           \
		stack-align.impl_gcc.main_gcc    \
		stack-align.impl_gcc.main_ucc    \
		stack-align.impl_ucc.main_gcc    \
		stack-align.impl_ucc.main_ucc    \
		stack-align.main.gcc.o           \
		stack-align.main.ucc.o
