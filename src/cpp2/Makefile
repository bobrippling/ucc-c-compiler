include ../src_config.mk

CFLAGS += -Wmissing-prototypes

OBJ = preproc.o main.o macro.o str.o \
      directive.o tokenise.o eval.o  \
      snapshot.o include.o deps.o    \
      expr.o expr_tok.o              \
      ../util/util.o ../util/alloc.o \
      ../util/platform.o             \
      ../util/where.o ../util/str.o  \
      ../util/dynarray.o             \
      ../util/dynmap.o               \
      ../util/std.o                  \
      ../util/escape.o               \
      ../util/warn.o                 \
      ../util/path.o

cpp: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}

clean:
	rm -f *.o cpp

.PHONY: clean deps

deps: Makefile.dep

Makefile.dep:
	${CC} ${CPPFLAGS} -MM *.c > $@

-include Makefile.dep
