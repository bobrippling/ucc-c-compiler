CFLAGS = -std=c99 -g
LDFLAGS = -g

XCCFLAGS = ${CFLAGS} -fcall-used-rbx

UCC = ./ucc
XCC = ${CC}
LD = ${CC}

TARGETS = $T.ucc_i.xcc_c $T.ucc_i.ucc_c $T.xcc_i.xcc_c $T.xcc_i.ucc_c

MACRO = IMPL

.PHONY: T tcheck clean

T: tcheck
	make -f abi.mk ${TARGETS}

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
	rm -f ${TARGETS}

%.ucc_i.xcc_c: %.ucc_i.o %.xcc_c.o
	${LD} ${LDFLAGS} -o $@ $^
%.xcc_i.ucc_c: %.xcc_i.o %.ucc_c.o
	${LD} ${LDFLAGS} -o $@ $^
%.ucc_i.ucc_c: %.ucc_i.o %.ucc_c.o
	${LD} ${LDFLAGS} -o $@ $^
%.xcc_i.xcc_c: %.xcc_i.o %.xcc_c.o
	${LD} ${LDFLAGS} -o $@ $^

%.xcc_c.o: %.c
	${XCC} ${XCCFLAGS} -c -o $@ $<
%.xcc_i.o: %.c
	${XCC} ${XCCFLAGS} -c -D${MACRO} -o $@ $<
%.ucc_c.o: %.c
	${UCC} ${CFLAGS} -c -o $@ $<
%.ucc_i.o: %.c
	${UCC} ${CFLAGS} -c -D${MACRO} -o $@ $<
