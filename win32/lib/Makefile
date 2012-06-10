CC = ../../src/ucc/ucc

crt.obj: crt.s_32
	nasm -f win32 -o $@ $<

crt.s_32: crt.s
	../nasm_64_to_32 < $< > $@

crt.s: crt.c
	${CC} -S -o $@ $<

clean:
	rm -f crt.s crt.s_32 crt.obj

.PHONY: clean
