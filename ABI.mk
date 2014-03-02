UCC = ./ucc
LD = ${CC}

TARGETS = $T.ucc_i.xcc_c $T.ucc_i.ucc_c $T.xcc_i.xcc_c $T.xcc_i.ucc_c

MACRO = IMPL

.PHONY: T tcheck clean

T: tcheck
	make -f ABI.mk ${TARGETS}

run: T
	for t in ${TARGETS}; do ./$$t || break; done

tcheck:
	@if test -z "$T"; then echo >&2 "Need target / \$$T"; false; fi

clean: tcheck
	rm -f ${TARGETS}

%.ucc_i.xcc_c: %.ucc_i.o %.xcc_c.o
	${LD} -o $@ $^
%.xcc_i.ucc_c: %.xcc_i.o %.ucc_c.o
	${LD} -o $@ $^
%.ucc_i.ucc_c: %.ucc_i.o %.ucc_c.o
	${LD} -o $@ $^
%.xcc_i.xcc_c: %.xcc_i.o %.xcc_c.o
	${LD} -o $@ $^

%.xcc_c.o: %.c
	${CC} -c -o $@ $<
%.xcc_i.o: %.c
	${CC} -c -D${MACRO} -o $@ $<
%.ucc_c.o: %.c
	${UCC} -c -o $@ $<
%.ucc_i.o: %.c
	${UCC} -c -D${MACRO} -o $@ $<
