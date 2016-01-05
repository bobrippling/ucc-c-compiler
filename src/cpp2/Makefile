include ../config.mk

CFLAGS += -Wmissing-prototypes

OBJ = preproc.o main.o macro.o str.o \
      directive.o tokenise.o eval.o  \
      snapshot.o include.o deps.o    \
      expr.o expr_tok.o has.o        \
      ../util/util.o ../util/alloc.o \
      ../util/platform.o             \
      ../util/where.o ../util/str.o  \
      ../util/dynarray.o             \
      ../util/dynmap.o               \
      ../util/std.o                  \
      ../util/escape.o               \
      ../util/warn.o                 \
      ../util/path.o

SRC = ${OBJ:.o=.c}

all: cpp tags

cpp: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f ${OBJ} cpp

.PHONY: clean all

include ../Makefile.common
