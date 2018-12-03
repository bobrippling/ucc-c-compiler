include ../config.mk

CFLAGS += -Wmissing-prototypes

OBJ_CPP = preproc.o main.o macro.o str.o \
      directive.o tokenise.o eval.o  \
      snapshot.o include.o deps.o    \
      expr.o expr_tok.o has.o

OBJ_DEP = ../util/util.o ../util/alloc.o \
      ../util/math.o                 \
      ../util/platform.o             \
      ../util/where.o ../util/str.o  \
      ../util/dynarray.o             \
      ../util/dynmap.o               \
      ../util/std.o                  \
      ../util/escape.o               \
      ../util/warn.o                 \
      ../util/io.o                   \
      ../util/path.o                 \
      ../util/triple.o

OBJ = ${OBJ_CPP} ${OBJ_DEP}

SRC = ${OBJ:.o=.c}

all: cpp tags

cpp: ${OBJ}
	@echo link $@
	$Q${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo clean cpp
	$Qrm -f ${OBJ_CPP} cpp

.PHONY: clean all

include ../Makefile.common
