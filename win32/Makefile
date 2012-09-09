tim: tim.obj
	ld -o tim \
		/cygdrive/c/Windows/System32/user32.dll \
		/cygdrive/c/Windows/System32/kernel32.dll \
		tim.obj

tim.obj: tim_32.s
	as -o $@ $<

tim_32.s: tim.s
	./nasm_64_to_32 < $< > $@

tim.s: tim.c ../src/cc1/cc1
	../ucc -fno-pic -S -o $@ $<

clean:
	rm -f tim tim.obj tim.s tim_32.s

.PHONY: clean
