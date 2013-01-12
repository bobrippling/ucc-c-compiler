UCC = ../../ucc
AS_64_to_32 = ../nasm_64_to_32

tim: tim_32.o jim_32.o printf.o /cygdrive/c/Windows/System32/user32.dll
	ld -e main -o $@ $^

%_32.s: %.s
	${AS_64_to_32} < $< > $@

%.s: %.c
	${UCC} -m32 -fno-pic -S -o $@ $<

clean:
	rm -f *.o tim.s jim.s tim jim_32.s tim_32.s
