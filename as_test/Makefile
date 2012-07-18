all: tim_crt_local tim_crt_lib

.PHONY: all

tim_crt_local: tim.o crt.o
	ld -o $@ $^

tim_crt_lib: tim.o ../lib/crt.o
	ld -o $@ $^

crt.o: crt.s
	as -o $@ $<

tim.o: tim.s
	as -o $@ $<
tim.s: tim.c
	../ucc -S -o $@ $<

clean:
	rm -f tim_crt_local tim_crt_lib tim.o crt.o tim.s
