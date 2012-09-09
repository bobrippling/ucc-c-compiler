LD_ARGS = /cygdrive/c/Windows/System32/user32.dll \
          /cygdrive/c/Windows/System32/kernel32.dll \

tim: tim.obj
	ld -o tim ${LD_ARGS} tim.obj

tim.obj: tim_32.s
	as -o $@ $<

tim_32.s: tim.s
	./nasm_64_to_32 < $< > $@

tim.s: tim.c
	../ucc -fno-pic -S -o $@ $<

clean:
	rm -f tim tim.obj tim.s tim_32.s working_32{,.o}

golden:
	as -o working_32.o working_32.s
	ld -o working_32 working_32.o ${LD_ARGS}

.PHONY: clean
