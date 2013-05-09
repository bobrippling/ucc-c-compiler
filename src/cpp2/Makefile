include ../src_config.mk

CFLAGS += -Wmissing-prototypes

OBJ = preproc.o main.o macro.o str.o \
			parse.o tokenise.o eval.o      \
			../util/util.o ../util/alloc.o \
			../util/platform.o ../util/dynarray.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean

eval.o: eval.c eval.h ../util/dynarray.h ../util/dyn.h ../util/util.h \
 ../util/alloc.h tokenise.h macro.h str.h main.h
macro.o: macro.c ../util/util.h ../util/alloc.h ../util/dynarray.h \
 ../util/dyn.h macro.h main.h
main.o: main.c ../util/util.h ../util/dynarray.h ../util/dyn.h \
 ../util/alloc.h ../util/platform.h main.h macro.h preproc.h
parse.o: parse.c ../util/util.h ../util/dynarray.h ../util/dyn.h \
 ../util/alloc.h parse.h tokenise.h main.h macro.h preproc.h
preproc.o: preproc.c ../util/util.h ../util/alloc.h main.h preproc.h \
 parse.h eval.h
str.o: str.c str.h ../util/alloc.h ../util/util.h macro.h
tokenise.o: tokenise.c ../util/dynarray.h ../util/dyn.h ../util/alloc.h \
 ../util/util.h tokenise.h str.h
