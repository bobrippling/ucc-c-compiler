include ../src_config.mk

CFLAGS += -Wmissing-prototypes

OBJ = preproc.o main.o macro.o str.o \
      directive.o tokenise.o eval.o  \
      snapshot.o include.o           \
      expr.o expr_tok.o              \
      ../util/util.o ../util/alloc.o \
      ../util/platform.o ../util/dynarray.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean

directive.o: directive.c ../util/util.h ../util/dynarray.h ../util/dyn.h \
 ../util/alloc.h directive.h tokenise.h main.h macro.h preproc.h \
 include.h str.h eval.h expr.h
eval.o: eval.c eval.h ../util/dynarray.h ../util/dyn.h ../util/util.h \
 ../util/alloc.h tokenise.h macro.h str.h main.h snapshot.h preproc.h
expr.o: expr.c expr.h expr_tok.h main.h ../util/alloc.h ../util/util.h \
 preproc.h
expr_tok.o: expr_tok.c expr.h expr_tok.h str.h ../util/alloc.h
include.o: include.c ../util/dynarray.h ../util/dyn.h ../util/alloc.h \
 include.h
macro.o: macro.c ../util/util.h ../util/alloc.h ../util/dynarray.h \
 ../util/dyn.h macro.h main.h preproc.h
main.o: main.c ../util/util.h ../util/dynarray.h ../util/dyn.h \
 ../util/alloc.h ../util/platform.h main.h macro.h preproc.h include.h
preproc.o: preproc.c ../util/util.h ../util/alloc.h main.h preproc.h \
 directive.h eval.h str.h
snapshot.o: snapshot.c ../util/dynarray.h ../util/dyn.h ../util/alloc.h \
 snapshot.h macro.h
str.o: str.c str.h ../util/alloc.h ../util/util.h macro.h
tokenise.o: tokenise.c ../util/dynarray.h ../util/dyn.h ../util/alloc.h \
 ../util/util.h tokenise.h str.h
