tim: tim.obj
	ld -o tim \
		/cygdrive/c/Windows/System32/user32.dll \
		/cygdrive/c/Windows/System32/kernel32.dll \
		tim.obj

tim.obj: tim.s_32
	nasm -f win32 -o $@ $<

tim.s_32: tim.s
	./nasm_64_to_32 < $< > $@

tim.s: tim.c
	../ucc -S -o $@ $<

clean:
	rm -f tim tim.obj tim.s tim.s_32

.PHONY: clean
