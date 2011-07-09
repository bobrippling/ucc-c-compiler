CFLAGS  = -g -Wall -Wextra -pedantic
LDFLAGS = -g -lm

cc1: tokenise.o parse.o cc1.o alloc.o util.o str.o tokconv.o gen.o gen_str.o tree.o
	${CC} ${LDFLAGS} -o $@ $^

clean:
	rm -f *.o cc1

test: cc1
	./cc1 INPUT.c

.PHONY: clean test

alloc.o: alloc.c alloc.h util.h
cc1.o: cc1.c tokenise.h util.h tree.h parse.h gen.h
gen_asm.o: gen_asm.c tree.h macros.h
gen.o: gen.c tree.h gen_str.h
gen_str.o: gen_str.c tree.h macros.h
parse.o: parse.c tokenise.h tree.h parse.h alloc.h tokconv.h util.h
str.o: str.c
tokconv.o: tokconv.c tokenise.h tree.h tokconv.h util.h macros.h
tokenise.o: tokenise.c tokenise.h alloc.h util.h str.h
tree.o: tree.c alloc.h tree.h macros.h
util.o: util.c util.h alloc.h
