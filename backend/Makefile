OBJ = val.o main.o mem.o isn.o op.o \
      dynmap.o

SRC = ${OBJ:.o=.c}

CFLAGS = -g
LDFLAGS = -g

all: tags val tim

tim: tim.ll
	llc -o $@ $< 2>.log; ec=$$?; sed 's/^llc: *//' .log; exit $$ec

val: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

tags: ${SRC}
	ctags ${SRC}

clean:
	rm -f val ${OBJ}

Makefile.dep: ${SRC}
	${CC} -MM ${SRC} > $@

-include Makefile.dep

.PHONY: clean all
