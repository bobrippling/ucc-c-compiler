CFLAGS = -std=c99 -g
LDFLAGS = -g

XCCFLAGS = ${CFLAGS}

UCC = ./ucc
XCC = ${CC}
LD = ${CC}

UCC_DEP = ./src/cc1/cc1

TARGETS = $T.ucc_i.xcc_c $T.ucc_i.ucc_c $T.xcc_i.xcc_c $T.xcc_i.ucc_c

MACRO = IMPL

OBJS = $T*.o

.PHONY: T tcheck clean
.SECONDARY: ${OBJS}

T: tcheck
	make -f ${MAKEFILE_LIST} ${TARGETS}

run: T
	for t in ${TARGETS}; \
	do \
		echo running $$t; \
		./$$t; \
		r=$$?; \
		if test $$r -ge 126 && test $$r -lt 157; then \
			exit $$r; \
		fi \
	done

tcheck:
	@if test -z "$T"; then echo >&2 "Need target / \$$T"; false; fi

clean: tcheck
	rm -f ${TARGETS} ${OBJS}

%.ucc_i.xcc_c: %.ucc_i.o %.xcc_c.o
	${LD} ${LDFLAGS} -o $@ $^
%.xcc_i.ucc_c: %.xcc_i.o %.ucc_c.o
	${LD} ${LDFLAGS} -o $@ $^
%.ucc_i.ucc_c: %.ucc_i.o %.ucc_c.o
	${LD} ${LDFLAGS} -o $@ $^
%.xcc_i.xcc_c: %.xcc_i.o %.xcc_c.o
	${LD} ${LDFLAGS} -o $@ $^

%.xcc_c.o: %.c ${UCC_DEP}
	${XCC} ${XCCFLAGS} -c -o $@ $<
%.xcc_i.o: %.c ${UCC_DEP}
	${XCC} ${XCCFLAGS} -c -D${MACRO} -o $@ $<
%.ucc_c.o: %.c ${UCC_DEP}
	${UCC} ${CFLAGS} -c -o $@ $<
%.ucc_i.o: %.c ${UCC_DEP}
	${UCC} ${CFLAGS} -c -D${MACRO} -o $@ $<
