NASM = /Users/rpilling/src/bin/nasm


float_arith: float_arith.o
	ld -e _start -o $@ $<

float_arith.o: float_arith.s
	${NASM} -f macho64 -o $@ $<
