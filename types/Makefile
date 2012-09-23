AS = as


float_arith: float_arith.o
	ld -e _start -o $@ $<

float_arith.o: float_arith.s
	${AS} -f macho64 -o $@ $<
